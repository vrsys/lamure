
namespace lamure {
namespace math {

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline matrix_t<scalar_t, row_dim, col_dim>& 
operator+=(      matrix_t<scalar_t, row_dim, col_dim>& lhs,
           const matrix_t<scalar_t, row_dim, col_dim>& rhs) {
    for (uint32_t i = 0; i < (row_dim * col_dim); ++i) {
        lhs.data_[i] += rhs.data_[i];
    }
    return (lhs);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline matrix_t<scalar_t, row_dim, col_dim>&
operator-=(      matrix_t<scalar_t, row_dim, col_dim>& lhs,
           const matrix_t<scalar_t, row_dim, col_dim>& rhs) {
    for (uint32_t i = 0; i < (row_dim * col_dim); ++i) {
        lhs.data_[i] -= rhs.data_[i];
    }
    return (lhs);
}

template<typename scalar_t, const uint32_t dim>
inline matrix_t<scalar_t, dim, dim>&
operator*=(      matrix_t<scalar_t, dim, dim>& lhs,
           const matrix_t<scalar_t, dim, dim>& rhs) {
    matrix_t<scalar_t, dim, dim> tmp_ret;

    uint32_t    dst_off;
    uint32_t    row_off;
    uint32_t    col_off;

    scalar_t   tmp_dp;

    for (uint32_t c = 0; c < dim; ++c) {
        for (uint32_t r = 0; r < dim; ++r) {
            dst_off = r + dim * c;
            tmp_dp = scalar_t(0);

            for (uint32_t d = 0; d < dim; ++d) {
                row_off = r + d * dim;
                col_off = d + c * dim;
                tmp_dp += lhs.data_[row_off] * rhs.data_[col_off];
            }

            tmp_ret.data_[dst_off] = tmp_dp;
        }
    }

    lhs = tmp_ret;

    return (lhs);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline matrix_t<scalar_t, row_dim, col_dim>&
operator*=(matrix_t<scalar_t, row_dim, col_dim>&   lhs,
           const scalar_t                     rhs) {
    for (uint32_t i = 0; i < (row_dim * col_dim); ++i) {
        lhs.data_[i] *= rhs;
    }
    return (lhs);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline matrix_t<scalar_t, row_dim, col_dim>&
operator/=(matrix_t<scalar_t, row_dim, col_dim>& lhs,
           const scalar_t rhs) {
    for (uint32_t i = 0; i < (row_dim * col_dim); ++i) {
        lhs.data_[i] /= rhs;
    }
    return (lhs);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline bool operator==(const matrix_t<scalar_t, row_dim, col_dim>& lhs,
           const matrix_t<scalar_t, row_dim, col_dim>& rhs) {
    bool return_value = true;

    for (uint32_t i = 0; (i < (row_dim * col_dim)) && return_value; ++i) {
        return_value = (return_value && (lhs.data_[i] == rhs.data_[i]));
    }

    return (return_value);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline bool operator!=(const matrix_t<scalar_t, row_dim, col_dim>& lhs,
           const matrix_t<scalar_t, row_dim, col_dim>& rhs) {
    bool return_value = false;

    for (uint32_t i = 0; (i < (row_dim * col_dim)) && !return_value; ++i) {
        return_value = (return_value || (lhs.data_[i] != rhs.data_[i]));
    }

    return (return_value);
}

template<typename scalar_t, const uint32_t dim>
inline const matrix_t<scalar_t, dim, dim>
operator*(const matrix_t<scalar_t, dim, dim>& lhs,
          const matrix_t<scalar_t, dim, dim>& rhs) {
    matrix_t<scalar_t, dim, dim> tmp(lhs);

    tmp *= rhs;

    return (tmp);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline const matrix_t<scalar_t, row_dim, col_dim>
operator*(const matrix_t<scalar_t, row_dim, col_dim>& lhs,
          const scalar_t                         rhs) {
    matrix_t<scalar_t, row_dim, col_dim> tmp(lhs);

    tmp *= rhs;

    return (tmp);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline const matrix_t<scalar_t, row_dim, col_dim>
operator/(const matrix_t<scalar_t, row_dim, col_dim>& lhs,
          const scalar_t                         rhs) {
    matrix_t<scalar_t, row_dim, col_dim> tmp(lhs);

    tmp /= rhs;

    return (tmp);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline const matrix_t<scalar_t, row_dim, col_dim>
operator+(const matrix_t<scalar_t, row_dim, col_dim>& lhs,
          const matrix_t<scalar_t, row_dim, col_dim>& rhs) {
    matrix_t<scalar_t, row_dim, col_dim> tmp(lhs);

    tmp += rhs;

    return (tmp);
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline const matrix_t<scalar_t, row_dim, col_dim>
operator-(const matrix_t<scalar_t, row_dim, col_dim>& lhs,
          const matrix_t<scalar_t, row_dim, col_dim>& rhs) {
    matrix_t<scalar_t, row_dim, col_dim> tmp(lhs);

    tmp -= rhs;

    return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const matrix_t<scalar_t, dim, dim>& lhs,
          const vector_t<scalar_t, dim>&        rhs) {
    vector_t<scalar_t, dim> tmp_ret(scalar_t(0));

    uint32_t    row_off;

    scalar_t   tmp_dp;

    for (uint32_t r = 0; r < dim; ++r) {
        tmp_dp = scalar_t(0);

        for (uint32_t c = 0; c < dim; ++c) {
            row_off = r + c * dim;
            tmp_dp += lhs.data_[row_off] * rhs.data_[c];
        }

        tmp_ret.data_[r] = tmp_dp;
    }

    return (tmp_ret);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim - 1>
operator*(const matrix_t<scalar_t, dim, dim>& lhs,
          const vector_t<scalar_t, dim - 1>&    rhs) {
    vector_t<scalar_t, dim - 1> tmp_ret(scalar_t(0));

    uint32_t    row_off;

    scalar_t   tmp_dp;

    for (uint32_t r = 0; r < dim - 1; ++r) {
        tmp_dp = scalar_t(0);

        for (uint32_t c = 0; c < dim - 1; ++c) {
            row_off = r + c * dim;
            tmp_dp += lhs.data_[row_off] * rhs.data_[c];
        }

        // w == 1
        tmp_ret.data_[r] = tmp_dp + lhs.data_[r + (dim - 1) * dim];
    }

    return (tmp_ret);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const vector_t<scalar_t, dim>&        lhs,
          const matrix_t<scalar_t, dim, dim>& rhs) {
    vector_t<scalar_t, dim> tmp_ret(scalar_t(0));

    uint32_t    row_off;

    scalar_t   tmp_dp;

    for (uint32_t r = 0; r < dim; ++r) {
        tmp_dp = scalar_t(0);

        for (uint32_t c = 0; c < dim; ++c) {
            row_off = r * dim + c;
            tmp_dp += rhs.data_[row_off] * lhs.data_[c];
        }

        tmp_ret.data_[r] = tmp_dp;
    }

    return (tmp_ret);
}

template<typename scalar_t, const uint32_t dim>
inline const matrix_t<scalar_t, dim, dim>
identity() {
    matrix_t<scalar_t, dim, dim> tmp_ret;
    for (uint32_t i = 0; i < dim*dim; ++i) {
        tmp_ret.data_[i] = (i % (dim + 1)) == 0 ? scalar_t(1) : scalar_t(0);
    }
    return tmp_ret;
}

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
inline const matrix_t<scalar_t, row_dim, col_dim>
transpose(const matrix_t<scalar_t, row_dim, col_dim>& lhs) {
    matrix_t<scalar_t, col_dim, row_dim> tmp_ret;

    uint32_t src_off;
    uint32_t dst_off;

    for (uint32_t c = 0; c < col_dim; ++c) {
        for (uint32_t r = 0; r < row_dim; ++r) {
            src_off = r + c * row_dim;
            dst_off = c + r * col_dim;

            tmp_ret.data_[dst_off] = lhs.data_[src_off];
        }
    }
    
    return (tmp_ret);
}

template<typename scalar_t, const uint32_t dim>
inline const matrix_t<scalar_t, dim, dim>
inverse(const matrix_t<scalar_t, dim, dim>& _lhs) {

    matrix_t<scalar_t, dim, dim> m;
    //matrix_t<scalar_t, dim, dim*2> a;
    scalar_t a[dim*dim*2];
    uint32_t dim2 = dim*2;

    //create Nx2N matrix A = (m0|E) for gauﬂ-jordan algorithm
    for (uint32_t row = 0; row < dim; ++row) {
        for (uint32_t column = 0; column < dim; ++column) {
            a[row*dim2+column] = _lhs.data_[row*dim+column];
        }
        for (uint32_t column = dim; column < 2*dim; ++column) {
            a[row*dim2+column] = row == column-dim ? static_cast<scalar_t>(1) : static_cast<scalar_t>(0);
        }
    }

    
    //gauﬂ algorithm
    for (uint32_t k = 0; k < dim-1; ++k) {
        //swap rows if pivot == 0
        if (a[k*dim2+k] == 0.f) {
            for (uint32_t i = k+1; i < dim; ++i) {
                if (a[i*dim2+k] != 0.f) {
                    //swap rows k and i
                    for (uint32_t column = 0; column < dim*2; ++column) {
                        scalar_t t = a[k*dim2+column];
                        a[k*dim2+column] = a[i*dim2+column];
                        a[i*dim2+column] = t;
                    }
                    break;
                }
                else if (i == dim-1) {
                    LAMURE_ASSERT(false, "gauss algorithm failure");
                }
            }
        }

        //eliminate entries below pivot
        for (uint32_t i = k+1; i < dim; ++i) {
            scalar_t t = a[i*dim2+k] / a[k*dim2+k];
            for (uint32_t j = k; j < dim2; ++j) {
                a[i*dim2+j] -= t * a[k*dim2+j];
            }
        }
    }

    
    //calc determinant
    scalar_t determinant = 1.0;
    for (uint32_t k = 0; k < dim; ++k) {
        determinant *= a[k*dim2+k];
    }
    
    LAMURE_ASSERT(determinant != static_cast<scalar_t>(0), "determinant is zero");
    
    //jordan algorithm
    for (uint32_t k = dim-1; k > 0; --k) {
        for (int32_t i = k-1; i >= 0; --i) {
            scalar_t t = a[i*dim2+k] / a[k*dim2+k];
            for (uint32_t j = k; j < dim2; ++j) {
                a[i*dim2+j] -= t * a[k*dim2+j];
            }
        }
    }

    //normalize the left half and store
    for (uint32_t i = 0; i < dim; ++i) {
        scalar_t t = static_cast<scalar_t>(1) / a[i*dim2+i];
        for (uint32_t j = dim; j < dim2; ++j) {
            m.data_[i*dim+(j-dim)] = t * a[i*dim2+j];
        }
    }

    return m;

}

}
}
