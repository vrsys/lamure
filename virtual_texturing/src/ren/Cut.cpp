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
Cut::Cut(pre::AtlasFile * atlas, CutState *front, CutState *back) : DoubleBuffer<CutState>(front, back)
{
    _atlas = atlas;
    _drawn = false;
}
void Cut::deliver() { _front->accept((*_back)); }
Cut &Cut::init_cut(pre::AtlasFile * atlas)
{
    uint32_t length_of_depth = (uint32_t)QuadTree::get_length_of_depth(atlas->getDepth() - 1) * 4;

    CutState *front_state = new CutState(length_of_depth);
    CutState *back_state = new CutState(length_of_depth);

    Cut * cut = new Cut(atlas, front_state, back_state);
    return *cut;
}
pre::AtlasFile *Cut::get_atlas() const { return _atlas; }
bool Cut::is_drawn() const { return _drawn; }
void Cut::set_drawn(bool _drawn) { Cut::_drawn = _drawn; }
uint32_t Cut::get_dataset_id(uint64_t cut_id) { return (uint32_t)(cut_id >> 32); }
uint16_t Cut::get_view_id(uint64_t cut_id) { return (uint16_t)(cut_id >> 16); }
uint16_t Cut::get_context_id(uint64_t cut_id) { return (uint16_t)cut_id; }
}