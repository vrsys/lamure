#ifndef LAMURE_RENDERER_H
#define LAMURE_RENDERER_H


// #include <lamure/utils.h>
// #include <lamure/types.h>
#include "utils.h"
#include "Scene.h"

#include <iostream>

#include <scm/core.h>
#include <scm/log.h>
#include <scm/core/pointer_types.h>
#include <scm/core/io/tools.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/viewer/camera.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/box.h>

#include <scm/core/math.h>

#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/gl_util/primitives/geometry.h>

#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>

#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/geometry.h>

#include <lamure/ren/controller.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/policy.h>

#include <boost/assign/list_of.hpp>
#include <memory>

#include <fstream>
#include <vector>
#include <map>
#include <lamure/types.h>
#include <lamure/utils.h>

#include <lamure/ren/cut.h>
#include <lamure/ren/cut_update_pool.h>


class Renderer {
	private:
        scm::shared_ptr<scm::gl::render_context> _context;
        scm::shared_ptr<scm::gl::render_device> _device;
        scm::gl::program_ptr _program_points;
        scm::gl::program_ptr _program_points_dense;
        scm::gl::program_ptr _program_cameras;
        scm::gl::program_ptr _program_images;
        scm::gl::program_ptr _program_frustra;

        scm::gl::rasterizer_state_ptr _rasterizer_state;
        int _width_window;
        int _height_window;

		lamure::ren::camera* _camera = new lamure::ren::camera();

		void draw_points_sparse(Scene scene);
		void draw_cameras(Scene scene);
		void draw_images(Scene scene);
		void draw_frustra(Scene scene);
		void draw_points_dense(Scene scene);
	public:
		Renderer();
		void init(char** argv, scm::shared_ptr<scm::gl::render_device> device, int width_window, int height_window);
		void render(Scene scene);
		
		bool mode_draw_points_dense = false;
		bool mode_draw_images = true;
};

#endif //LAMURE_RENDERER_H
