#include <lamure/vt/ren/CutDatabase.h>

namespace vt
{
CutState::CutState(uint32_t _size_index_buffer) : _cut(), _mem_slots_updated(), _mem_slots_locked()
{
    this->_size_index_buffer = _size_index_buffer;

    _index = new uint8_t[_size_index_buffer];
    std::fill(_index, _index + _size_index_buffer, 0);

    _cut.insert(0);
}
void CutState::accept(CutState &cut_state)
{
    _cut.clear();
    _cut.insert(cut_state._cut.begin(), cut_state._cut.end());

    _mem_slots_locked = mem_slots_index_type(cut_state._mem_slots_locked);
    _mem_slots_updated = mem_slots_index_type(cut_state._mem_slots_updated);

    std::copy(cut_state._index, cut_state._index + _size_index_buffer, _index);
}
CutState::~CutState() { delete _index; }
cut_type &CutState::get_cut() { return _cut; }
uint8_t *CutState::get_index() { return _index; }
mem_slots_index_type &CutState::get_mem_slots_updated() { return _mem_slots_updated; }
mem_slots_index_type &CutState::get_mem_slots_locked() { return _mem_slots_locked; }
Cut::Cut(TileAtlas<priority_type> *atlas, uint16_t max_depth, uint32_t size_index_texture, CutState *front, CutState *back) : DoubleBuffer<CutState>(front, back)
{
    _atlas = atlas;
    _max_depth = max_depth;
    _size_index_texture = size_index_texture;
    _drawn = false;
}
void Cut::deliver() { _front->accept((*_back)); }
uint16_t Cut::identify_max_depth(const std::string &file_name)
{
    size_t fsize = 0;
    std::ifstream file;

    file.open(file_name, std::ifstream::in | std::ifstream::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("Mipmap could not be read");
    }

    fsize = (size_t)file.tellg();
    file.seekg(0, std::ios::end);
    fsize = (size_t)file.tellg() - fsize;

    file.close();

    auto texel_count = (uint32_t)(size_t)fsize / VTConfig::get_instance().get_byte_stride();

    size_t count_tiled = texel_count / VTConfig::get_instance().get_size_tile() / VTConfig::get_instance().get_size_tile();

    return (uint16_t)QuadTree::get_depth_of_node(count_tiled - 1);
}
Cut &Cut::init_cut(const std::string &file_name)
{
    uint16_t max_depth = Cut::identify_max_depth(file_name);
    uint32_t size_index_texture = Cut::identify_size_index_texture(max_depth);

    CutState *front_state = new CutState(size_index_texture * size_index_texture * 4);
    CutState *back_state = new CutState(size_index_texture * size_index_texture * 4);

    if(access((file_name).c_str(), F_OK) != -1)
    {
        std::runtime_error("Mipmap file not found: " + file_name);
    }

    uint32_t size_tile = vt::VTConfig::get_instance().get_size_tile();
    auto *atlas = new vt::TileAtlas<vt::priority_type>((std::string &)file_name, size_tile * size_tile * vt::VTConfig::get_instance().get_byte_stride());

    Cut * cut = new Cut(atlas, max_depth, size_index_texture, front_state, back_state);
    return *cut;
}
TileAtlas<priority_type> *Cut::get_atlas() const { return _atlas; }
uint16_t Cut::get_max_depth() const { return _max_depth; }
uint32_t Cut::get_size_index_texture() const { return _size_index_texture; }
bool Cut::is_drawn() const { return _drawn; }
void Cut::set_drawn(bool _drawn) { Cut::_drawn = _drawn; }
uint32_t Cut::get_dataset_id(uint64_t cut_id) { return (uint32_t)cut_id >> 32; }
uint16_t Cut::get_view_id(uint64_t cut_id) { return (uint16_t)cut_id >> 16; }
uint16_t Cut::get_context_id(uint64_t cut_id) { return (uint16_t)cut_id; }
}