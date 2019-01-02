

// #ifndef LAMURE_SYMMAT_H
// #define LAMURE_SYMMAT_H

#include <vector>


#include <CGAL/Simple_cartesian.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Point_3<Kernel> Point;

class SymMat {
	
private:
	std::vector<double> elements;
public:
	SymMat();

	SymMat(Point vertex);

	SymMat operator+(const SymMat& rhs) const;


};

// #endif

