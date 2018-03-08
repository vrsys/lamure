//
// Created by towe2387 on 12/10/17.
//

#ifndef LAMURE_CUT_H
#define LAMURE_CUT_H

#include <lamure/vt/common.h>
#include <lamure/vt/ren/DoubleBuffer.h>
namespace vt
{
class CutDatabase;
class CutState
{
  public:
    explicit CutState(CutDatabase *cut_db);
    ~CutState();

    uint8_t *get_index();
    cut_type &get_cut();
    mem_slots_index_type &get_mem_slots_updated();
    mem_slots_index_type &get_mem_slots_locked();
    void accept(CutState &cut_state);

  private:
    const CutDatabase *_cut_db;

    uint8_t *_index;
    cut_type _cut;
    mem_slots_index_type _mem_slots_updated;
    mem_slots_index_type _mem_slots_locked;
};

class Cut : public DoubleBuffer<CutState>
{
  public:
    explicit Cut(CutDatabase *cut_db, CutState *front, CutState *back);
    ~Cut() override{};

  protected:
    void deliver() override;

  private:
    const CutDatabase *_cut_db;
};
}

#endif // LAMURE_CUT_H
