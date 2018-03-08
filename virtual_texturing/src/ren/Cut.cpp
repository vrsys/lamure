#include <lamure/vt/ren/CutDatabase.h>

namespace vt
{
CutState::CutState(CutDatabase *cut_db) : _cut(), _mem_slots_updated(), _mem_slots_locked()
{
    _cut_db = cut_db;
    _index = new uint8_t[_cut_db->get_size_index()];

    std::fill(_index, _index + _cut_db->get_size_index(), 0);

    _cut.insert(0);
}
void CutState::accept(CutState &cut_state)
{
    _cut.clear();
    _cut.insert(cut_state._cut.begin(), cut_state._cut.end());

    _mem_slots_locked = mem_slots_index_type(cut_state._mem_slots_locked);
    _mem_slots_updated = mem_slots_index_type(cut_state._mem_slots_updated);

    std::copy(cut_state._index, cut_state._index + _cut_db->get_size_index(), _index);
}
CutState::~CutState() { delete _index; }
cut_type &CutState::get_cut() { return _cut; }
uint8_t *CutState::get_index() { return _index; }
mem_slots_index_type &CutState::get_mem_slots_updated() { return _mem_slots_updated; }
mem_slots_index_type &CutState::get_mem_slots_locked() { return _mem_slots_locked; }
Cut::Cut(CutDatabase *cut_db, CutState *front, CutState *back) : DoubleBuffer<CutState>(front, back) { _cut_db = cut_db; }
void Cut::deliver() { _front->accept((*_back)); }
}