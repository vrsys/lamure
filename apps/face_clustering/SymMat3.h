#include <vector>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Point_3<Kernel> Point;

class SymMat3 {
	
private:
	std::vector<double> elements;
public:
	SymMat3() {
		elements = std::vector<double>(5,0);
	}

	SymMat3(Point vertex){
		elements[0] = (vertex.x() * vertex.x());
		elements[1] = (vertex.x() * vertex.y());
		elements[2] = (vertex.x() * vertex.z());
		elements[3] = (vertex.y() * vertex.y());
		elements[4] = (vertex.y()* vertex.z());
		elements[5] = (vertex.z() * vertex.z());
	}

	SymMat3 operator+(const SymMat3& rhs) const{
		SymMat3 result = *this;
		for (int i = 0; i < 6; ++i)
		{
			result.elements[i] += rhs.elements[i];
		}
		return result;
	}

}

