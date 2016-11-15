// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <algorithm>

#include <lamure/pvs/visibility_test.h>
#include <lamure/pvs/visibility_test_id_histogram_renderer.h>

#include <lamure/pvs/grid.h>
#include <lamure/pvs/regular_grid.h>

int main(int argc, char** argv)
{
    // Load scene and analyze scene size.
    lamure::pvs::visibility_test* vt = new lamure::pvs::visibility_test_id_histogram_renderer();
    vt->initialize(argc, argv);
    lamure::vec3r scene_dimensions = vt->get_scene_bounds().get_dimensions();

    // Create grid based on scene size.
    unsigned int num_cells = 16;
    double cell_size = (std::max(scene_dimensions.x, std::max(scene_dimensions.y, scene_dimensions.z)) / (double)num_cells) * 1.5;
    scm::math::vec3d center(vt->get_scene_bounds().get_center().x, vt->get_scene_bounds().get_center().y, vt->get_scene_bounds().get_center().z);
    lamure::pvs::grid* test_grid = new lamure::pvs::regular_grid(num_cells, cell_size, center);

    // Run visibility test on given scene and grid.
    vt->test_visibility(test_grid);
    vt->shutdown();

    delete test_grid;
    delete vt;

    return 0;
}
