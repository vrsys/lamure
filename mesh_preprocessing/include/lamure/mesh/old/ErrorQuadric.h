struct ErrorQuadric
{
    SymMat A;
    Vector b;
    double c;

    ErrorQuadric()
    {
        b = Vector(0, 0, 0);
        c = 0;
    }

    // for p quad
    ErrorQuadric(Point& p)
    {
        A = SymMat(p);
        b = Vector(p.x(), p.y(), p.z());
        c = 1;
    }

    // for r quad
    ErrorQuadric(const Vector& v)
    {
        A = SymMat(v);
        b = -v;
        c = 1;
    }

    ErrorQuadric operator+(const ErrorQuadric& e)
    {
        ErrorQuadric eq;
        eq.A = A + e.A;
        eq.b = b + e.b;
        eq.c = c + e.c;

        return eq;
    }

    ErrorQuadric operator*(const double rhs)
    {
        ErrorQuadric result = *this;

        result.A = result.A * rhs;
        result.b = result.b * rhs;
        result.c = result.c * rhs;

        return result;
    }

    // create covariance / 'Z' matrix
    // A - ( (b*bT) / c)
    void get_covariance_matrix(double cm[3][3])
    {
        SymMat rhs = SymMat(b) / c;
        SymMat result = A - rhs;
        result.to_c_mat3(cm);
    }

    std::string print()
    {
        std::stringstream ss;
        ss << "A:\n" << A.print_mat();
        ss << "b:\n[" << b.x() << " " << b.y() << " " << b.z() << "]" << std::endl;
        ss << "c:\n" << c << std::endl;
        return ss.str();
    }
};