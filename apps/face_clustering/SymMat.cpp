#include "SymMat.h"

SymMat SymMat::operator+(const SymMat& rhs) const{
	SymMat result = *this;
	for (int i = 0; i < 6; ++i)
	{
		result.elements[i] += rhs.elements[i];
	}
	return result;
}

SymMat SymMat::operator-(const SymMat& rhs) const{
	SymMat result = *this;
	for (int i = 0; i < 6; ++i)
	{
		result.elements[i] -= rhs.elements[i];
	}
	return result;
}

SymMat SymMat::operator/(const double rhs) const{

	if (rhs == 0.0)
	{
		std::cout << "ERROR: divide by 0 (in SymMat operator/)\n";
	}

	SymMat result = *this;
	for (int i = 0; i < 6; ++i)
	{
		result.elements[i] /= rhs;
	}
	return result;
}


Vector SymMat::operator*(const Vector& rhs) const {
	SymMat lhs = *this;

	double x = (rhs.x() * lhs.elements[0]) + (rhs.y() * lhs.elements[1]) + (rhs.z() * lhs.elements[2]);
	double y = (rhs.x() * lhs.elements[1]) + (rhs.y() * lhs.elements[3]) + (rhs.z() * lhs.elements[4]);
	double z = (rhs.x() * lhs.elements[2]) + (rhs.y() * lhs.elements[4]) + (rhs.z() * lhs.elements[5]);
	return Vector(x,y,z);
}

SymMat SymMat::operator*(const double rhs) const{

	SymMat result = *this;
	for (int i = 0; i < 6; ++i)
	{
		result.elements[i] *= rhs;
	}
	return result;
}


SymMat::SymMat(){

	elements = std::vector<double>(6,0);
}

SymMat::SymMat(Point vertex){
	elements = std::vector<double>(6,0);
	elements[0] = (vertex.x() * vertex.x());
	elements[1] = (vertex.x() * vertex.y());
	elements[2] = (vertex.x() * vertex.z());
	elements[3] = (vertex.y() * vertex.y());
	elements[4] = (vertex.y()* vertex.z());
	elements[5] = (vertex.z() * vertex.z());
}

SymMat::SymMat(Vector vector){
	elements = std::vector<double>(6,0);
	elements[0] = (vector.x() * vector.x());
	elements[1] = (vector.x() * vector.y());
	elements[2] = (vector.x() * vector.z());
	elements[3] = (vector.y() * vector.y());
	elements[4] = (vector.y()* vector.z());
	elements[5] = (vector.z() * vector.z());
}

void SymMat::to_c_mat3(double cm[3][3]){

	cm[0][0] = elements[0];
	cm[0][1] = elements[1];
	cm[0][2] = elements[2];
	cm[1][0] = elements[1];
	cm[1][1] = elements[3];
	cm[1][2] = elements[4];
	cm[2][0] = elements[2];
	cm[2][1] = elements[4];
	cm[2][2] = elements[5];

}

std::string SymMat::print_mat(){
	// printf("[ %4.f %4.f %4.f]\n", elements[0],elements[1],elements[2]);
	// printf("[ %4.f %4.f %4.f]\n", elements[1],elements[3],elements[4]);
	// printf("[ %4.f %4.f %4.f]\n", elements[2],elements[4],elements[5]);

	std::stringstream ss;
	ss << std::fixed << std::setprecision(4) << "[" << elements[0] << ", " << elements[1] << ", " << elements[2]  << "]" << std::endl;
	ss << std::fixed << std::setprecision(4) << "[" << elements[1] << ", " << elements[3] << ", " << elements[4]  << "]" << std::endl;
	ss << std::fixed << std::setprecision(4) << "[" << elements[2] << ", " << elements[4] << ", " << elements[5]  << "]" << std::endl;

	return ss.str();
}
