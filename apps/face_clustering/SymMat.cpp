#include "SymMat.h"

SymMat SymMat::operator+(const SymMat& rhs) const{
	SymMat result = *this;
	for (int i = 0; i < 6; ++i)
	{
		result.elements[i] += rhs.elements[i];
	}
	return result;
}


SymMat::SymMat(){

	elements = std::vector<double>(5,0);
}

SymMat::SymMat(Point vertex){
	elements[0] = (vertex.x() * vertex.x());
	elements[1] = (vertex.x() * vertex.y());
	elements[2] = (vertex.x() * vertex.z());
	elements[3] = (vertex.y() * vertex.y());
	elements[4] = (vertex.y()* vertex.z());
	elements[5] = (vertex.z() * vertex.z());
}
