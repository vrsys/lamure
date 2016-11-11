#ifndef VISIBILITY_TEST_H
#define VISIBILITY_TEST_H

namespace lamure
{
namespace pvs
{

class visibility_test
{
public:
	virtual ~visibility_test() {}

	virtual int initialize(int& argc, char** argv) = 0;
	virtual void test_visibility() = 0;
	virtual void shutdown() = 0;
};

}
}

#endif
