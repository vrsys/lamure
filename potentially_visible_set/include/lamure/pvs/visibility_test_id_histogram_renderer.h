#ifndef LAMURE_PVS_VISIBILITY_TEST_ID_HISTOGRAM_RENDERER_H
#define LAMURE_PVS_VISIBILITY_TEST_ID_HISTOGRAM_RENDERER_H

#include "lamure/pvs/visibility_test.h"
#include "lamure/pvs/management.h"

namespace lamure
{
namespace pvs
{

class visibility_test_id_histogram_renderer : public visibility_test
{
public:
	visibility_test_id_histogram_renderer();
	~visibility_test_id_histogram_renderer();

	virtual int initialize(int& argc, char** argv);
	virtual void test_visibility();
	virtual void shutdown();

private:
	int resolution_x_;
	int resolution_y_;
	unsigned int main_memory_budget_;
    unsigned int video_memory_budget_;
    unsigned int max_upload_budget_;

    management* management_;
};

}
}

#endif
