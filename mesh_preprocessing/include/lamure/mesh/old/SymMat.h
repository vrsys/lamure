#include <vector>
#include <cstdio>
#include <string>
#include <sstream>

#include <CGAL/Simple_cartesian.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Point_3<Kernel> Point;
typedef Kernel::Vector_3 Vector;

class SymMat
{
  private:
    std::vector<double> elements;

  public:
    SymMat();

    SymMat(Point vertex);

    SymMat(Vector vector);

    SymMat operator+(const SymMat& rhs) const;

    SymMat operator-(const SymMat& rhs) const;

    SymMat operator/(const double rhs) const;

    Vector operator*(const Vector& rhs) const;

    SymMat operator*(const double rhs) const;

    void to_c_mat3(double cm[3][3]);

    std::string print_mat();
};