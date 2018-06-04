//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_RESOURCES_H
#define LAMURE_RESOURCES_H

struct resource {
    uint64_t num_primitives_ {0};
    scm::gl::buffer_ptr buffer_;
    scm::gl::vertex_array_ptr array_;
};

#endif //LAMURE_RESOURCES_H
