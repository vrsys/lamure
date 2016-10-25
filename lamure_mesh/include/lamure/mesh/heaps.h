
#ifndef LAMURE_MESH_HEAPS_H_
#define LAMURE_MESH_HEAPS_H_

#include <lamure/types.h>
#include <lamure/util/heap.h>
#include <lamure/mesh/chart.h>

namespace lamure {
namespace mesh {

struct indexed_edge_t {
  uint32_t id_;
  float32_t cost_;
};


typedef util::heap_base_t<indexed_edge_t> edge_heap_t;
typedef util::heap_base_t<chart_t> chart_heap_t;


}
}

#endif
