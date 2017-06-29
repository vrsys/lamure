#ifndef VISIBILITY_TEST_H
#define VISIBILITY_TEST_H

#include <lamure/pvs/pvs_preprocessing.h>
#include "lamure/bounding_box.h"
#include "lamure/pvs/grid.h"

namespace lamure
{
namespace pvs
{

class visibility_test
{
public:
	virtual ~visibility_test() {}

	virtual int initialize(int& argc, char** argv) = 0;
	virtual void test_visibility(grid* visibility_grid) = 0;
	virtual void shutdown() = 0;

	virtual bounding_box get_scene_bounds() const = 0;
};

}
}

#endif
