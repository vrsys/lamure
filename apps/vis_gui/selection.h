//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_SELECTION_H
#define LAMURE_SELECTION_H


#include <cstdint>
#include <set>

#include "xyz.h"

struct selection {
    int32_t selected_model_ = -1;
    std::vector<xyz> brush_;
    std::set<uint32_t> selected_views_;
};

#endif //LAMURE_SELECTION_H
