#ifndef LAMURE_FRUSTUM_H
#define LAMURE_FRUSTUM_H

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
#include <vector>

#include "Struct_Line.h"

class Frustum {

	private:
		scm::math::vec3f _near_left_top = scm::math::vec3f(-0.5, 0.5, 0.0);
		scm::math::vec3f _near_right_top = scm::math::vec3f(0.5, 0.5, 0.0);
		scm::math::vec3f _near_left_bottom = scm::math::vec3f(-0.5, -0.5, 0.0);
		scm::math::vec3f _near_right_bottom = scm::math::vec3f(0.5, -0.5, 0.0);
		scm::math::vec3f _far_left_top = scm::math::vec3f(-1.0, 1.0, 0.0);
		scm::math::vec3f _far_right_top = scm::math::vec3f(1.0, 1.0, 0.0);
		scm::math::vec3f _far_left_bottom = scm::math::vec3f(-1.0, -1.0, 0.0);
		scm::math::vec3f _far_right_bottom = scm::math::vec3f(1.0, -1.0, 0.0);
	    scm::gl::vertex_array_ptr _vertex_array_object;

		std::vector<Struct_Line> convert_frustum_to_struct_line();
	public:
		Frustum();
    	void init(scm::shared_ptr<scm::gl::render_device> device, std::vector<scm::math::vec3f> vertices);
	    scm::gl::vertex_array_ptr get_vertex_array_object();
};

#endif //LAMURE_FRUSTUM_H
