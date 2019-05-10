

// #include <lamure/mesh/triangle.h>

#ifndef TRIANGLE_CHARTID_H_
#define TRIANGLE_CHARTID_H_

namespace lamure
{
namespace mesh
{
class Triangle_Chartid : public lamure::mesh::triangle_t
{
  public:
    Triangle_Chartid() : lamure::mesh::triangle_t()
    {
        chart_id = -1;
        tex_id = 0;
        area_id = 0;
        tri_id = 0;
    }
    ~Triangle_Chartid() {}

    lamure::mesh::triangle_t get_basic_triangle()
    {
        lamure::mesh::triangle_t tri;
        tri.v0_ = v0_;
        tri.v1_ = v1_;
        tri.v2_ = v2_;
        return tri;
    }

    int chart_id;
    int tex_id;
    int area_id;
    int tri_id;
};

} // namespace mesh
} // namespace lamure

#endif