// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_PROVENANCE_H_
#define REN_PROVENANCE_H_

#include <lamure/types.h>
#include <lamure/utils.h>
#include <lamure/ren/Item_Provenance.h>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace lamure
{
namespace ren
{
class Data_Provenance
{
    public:
        Data_Provenance() {};

        void add_item(Item_Provenance item_provenance) {
            _items_provenance.push_back(item_provenance);
            _size_in_bytes += item_provenance.get_size_in_bytes();
        };

        std::vector<Item_Provenance> get_items() { return _items_provenance; };
        // int get_size_in_bytes() { return 4; };
        int get_size_in_bytes() { return 12; };
        // int get_size_in_bytes() { return 8; };
        // int get_size_in_bytes() { return _size_in_bytes; };
    private:
        std::vector<Item_Provenance> _items_provenance;
        int _size_in_bytes = 0;
};
}
} // namespace lamure

#endif // REN_PROVENANCE_H_