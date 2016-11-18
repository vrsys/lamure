// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_PVS_MANAGEMENT_H_
#define LAMURE_PVS_MANAGEMENT_H_

#include <lamure/types.h>

#include "lamure/pvs/renderer.h"
#include "lamure/pvs/grid.h"

#include <lamure/ren/config.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/controller.h>
#include <lamure/ren/cut.h>
#include <lamure/ren/cut_update_pool.h>

#include <GL/freeglut.h>

#include <FreeImagePlus.h>

#include <lamure/ren/ray.h>

namespace lamure
{
namespace pvs
{

class management
{
public:
                        management(std::vector<std::string> const& model_filenames,
                            std::vector<scm::math::mat4f> const& model_transformations,
                            const std::set<lamure::model_t>& visible_set,
                            const std::set<lamure::model_t>& invisible_set);
                        
    virtual             ~management();

                        management(const management&) = delete;
                        management& operator=(const management&) = delete;

    bool                MainLoop();
    void                update_trackball(int x, int y);
    void                RegisterMousePresses(int button, int state, int x, int y);
    void                dispatchKeyboardInput(unsigned char key);
    void                dispatchResize(int w, int h);

    void                SetSceneName();

    float               error_threshold_;
    void                IncreaseErrorThreshold();
    void                DecreaseErrorThreshold();

    void                set_grid(grid* visibility_grid);

protected:

    void                Toggledispatching();

    void                emit_node_visibility(grid* visibility_grid);
    void                set_node_parents_visible(const model_t& model_id, const node_t& node_id, view_cell* cell);
    void                set_node_children_visible(const model_t& model_id, const node_t& node_id, view_cell* cell);

private:
    lamure::ren::camera::mouse_state mouse_state_;

    Renderer* renderer_;

    lamure::ren::camera*   active_camera_;

    int32_t             width_;
    int32_t             height_;

    float importance_;
    float visibility_threshold_;

    bool test_send_rendered_;

    bool                dispatch_;

    lamure::model_t     num_models_;

    float               near_plane_;
    float               far_plane_;

    std::vector<scm::math::mat4f> model_transformations_;
    std::vector<std::string> model_filenames_;

    bool                first_frame_;

    grid*               visibility_grid_;
    unsigned int        current_grid_index_;
    unsigned int        direction_counter_;

    double              average_cut_update_time_;
    double              average_render_time_;
};

}
}

#endif
