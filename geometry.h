#pragma once

#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>

template <typename T, typename CRTP>
struct VecBase
{
    CRTP &crtp()
    {
        return static_cast<CRTP &>(*this);
    }
    CRTP const &crtp() const
    {
        return static_cast<CRTP const &>(*this);
    }

    size_t size() const
    {
        return std::extent<decltype(crtp().data)>::value;
    }

    void zero()
    {
        std::fill(std::begin(crtp().data), std::end(crtp().data), T());
    }

    T *begin()
    {
        return &crtp().data[0];
    }

    T *end()
    {
        return &crtp().data[0] + size();
    }

    T const *begin() const
    {
        return &crtp().data[0];
    }

    T const *end() const
    {
        return &crtp().data[0] + size();
    }

    T &operator[](size_t i)
    {
        assert(i < crtp().size());
        return crtp().data[i];
    }

    T const &operator[](size_t i) const
    {
        assert(i < crtp().size());
        return crtp().data[i];
    }
};

template <typename T, size_t N>
struct Vec : VecBase<T, Vec<T, N>>
{
    Vec() : data({0}) {};
    union
    {
        T data[N];
        struct
        {
            T x;
            T y;
            T z;
            T w;
        };
        struct
        {
            T r;
            T g;
            T b;
            T a;
        };
    };
};

template <typename T>
struct Vec<T, 2> : VecBase<T, Vec<T, 2>>
{
    Vec(T x, T y) : x(x), y(y) {};
    Vec() : data({0}) {};

    T wedgeComp(Vec<T, 2> const &other)
    {
        return x * other.y - y * other.x;
    }

    union
    {
        T data[2];
        struct
        {
            T x;
            T y;
        };
    };
};

template <typename T>
struct Vec<T, 3> : VecBase<T, Vec<T, 3>>
{
    Vec(T x, T y, T z) : x(x), y(y), z(z) {};
    Vec() : data({0}) {};

    union
    {
        T data[3];
        struct
        {
            T x;
            T y;
            T z;
        };
        struct
        {
            T r;
            T g;
            T b;
        };
    };
};

template <typename T>
struct Vec<T, 4> : VecBase<T, Vec<T, 4>>
{
    Vec(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {};
    Vec() : data({0}) {};

    union
    {
        T data[4];
        struct
        {
            T x;
            T y;
            T z;
            T w;
        };
        struct
        {
            T r;
            T g;
            T b;
            T a;
        };
    };
};

template <typename T, size_t N>
std::ostream &operator<<(std::ostream &os, Vec<T, N> const &v)
{
    os << "[ ";
    for (size_t i = 0; i < N; i++)
    {
        os << v[i] << " ";
    }
    os << "]";
    return os;
}

template <typename T, typename U, size_t N>
auto operator+(Vec<T, N> const &a, Vec<U, N> const &b) -> Vec<decltype(a[0] + b[0]), N>
{
    Vec<decltype(a[0] + b[0]), N> out;
    for (size_t i = 0; i < out.size(); i++)
    {
        out[i] = a[i] + b[i];
    }
    return out;
}

template <typename T, typename U, size_t N>
auto operator-(Vec<T, N> const &a, Vec<U, N> const &b) -> Vec<decltype(a[0] - b[0]), N>
{
    Vec<decltype(a[0] - b[0]), N> out;
    for (size_t i = 0; i < out.size(); i++)
    {
        out[i] = a[i] - b[i];
    }
    return out;
}

template <typename T, typename U, size_t N>
auto operator*(Vec<T, N> const &a, Vec<U, N> const &b) -> Vec<decltype(a[0] * b[0]), N>
{
    Vec<decltype(a[0] * b[0]), N> out;
    for (size_t i = 0; i < N; i++)
    {
        out[i] = a[i] * b[i];
    }
    return out;
}

template <typename T, typename U, size_t N>
auto dot(Vec<T, N> const &a, Vec<U, N> const &b) -> decltype(a[0] * b[0])
{
    auto product = a * b;
    return std::accumulate(std::begin(product), std::end(product), decltype(product.x)(0));
}

template <typename T, typename U>
auto cross(Vec<T, 3> const &a, Vec<U, 3> const &b) -> decltype(a[0] * b[0])
{
    Vec<decltype(a[0] * b[0]), 3> out;
    out.x = a.y * b.z - a.z * b.y;
    out.y = a.z * b.x - a.x * b.z;
    out.z = a.x * b.y - a.y * b.x;
    return out;
}

template <typename T, typename U, size_t N>
auto operator*(U const scalar, Vec<T, N> const &v) -> Vec<decltype(scalar * v[0]), N>
{
    Vec<decltype(scalar * v[0]), N> out;

    for (size_t i = 0; i < N; i++)
    {
        out[i] = scalar * v[i];
    }
    return out;
}
template <typename T, typename U, size_t N>
auto operator*(Vec<T, N> const &v, U const scalar) -> Vec<decltype(v[0] * scalar), N>
{
    Vec<decltype(v[0] * scalar), N> out;

    for (size_t i = 0; i < N; i++)
    {
        out[i] = v[i] * scalar;
    }
    return out;
}

typedef Vec<double, 2> vec2;
typedef Vec<double, 3> vec3;
typedef Vec<double, 4> vec4;

template <typename T, size_t N, size_t M, typename CRTP>
struct MatBase
{
    CRTP &crtp()
    {
        return static_cast<CRTP &>(*this);
    }

    CRTP const &crtp() const
    {
        return static_cast<CRTP const &>(*this);
    }

    size_t size() const
    {
        return std::extent<decltype(crtp().data)>::value;
    }

    size_t nRows() const
    {
        return N;
    }

    size_t nCols() const
    {
        return M;
    }

    void zero()
    {
        std::fill(std::begin(crtp().data), std::end(crtp().data), T());
    }

    T operator()(size_t const i, size_t const j) const
    {
        return crtp().data[i * M + j];
    }

    T &operator()(size_t const i, size_t const j)
    {
        return crtp().data[i * M + j];
    }

    Vec<T, M> row(size_t const i) const
    {
        Vec<T, M> out;
        for (size_t j = 0; j < M; j++)
        {
            out.data[j] = crtp().data[i * M + j];
        }
        return out;
    }

    Vec<T, N> col(size_t const j) const
    {
        Vec<T, N> out;
        for (size_t i = 0; i < N; i++)
        {
            out.data[i] = crtp().data[i * M + j];
        }
        return out;
    }
};

template <typename T, size_t N, size_t M>
struct Mat : MatBase<T, N, M, Mat<T, N, M>>
{
    Mat() : data({0}) {}

    template <typename... Ts>
    Mat(Ts... inputs) : data({0})
    {
        T temp[N * M] = {static_cast<T>(inputs)...};
        for (size_t i = 0; i < N * M; i++)
        {
            data[i] = temp[i];
        }
    }

    Mat<T, M, N> transpose() const
    {
        Mat<T, M, N> out;
        for (size_t i = 0; i < N; i++)
        {
            for (size_t j = 0; j < M; j++)
            {
                out.data[j * N + i] = data[i * M + j];
            }
        }
        return out;
    }

    union
    {
        T data[N * M];
        Vec<T, M> mRows[N];
    };
};

template <typename T>
struct Mat<T, 2, 2> : MatBase<T, 2, 2, Mat<T, 2, 2>>
{
    union
    {
        T data[4];
        struct
        {
            T a00, a01, a10, a11;
        };
    };
};

template <typename T>
struct Mat<T, 3, 3> : MatBase<T, 3, 3, Mat<T, 3, 3>>
{
    union
    {
        T data[9];
        struct
        {
            T m00, m01, m02, m10, m11, m12, m20, m21, m22;
        };
    };
};

template <typename T>
struct Mat<T, 4, 4> : MatBase<T, 4, 4, Mat<T, 4, 4>>
{
    union
    {
        T data[16];
        struct
        {
            T a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23, a30, a31, a32, a33;
        };
    };
};

template <typename T, size_t N, size_t M>
std::ostream &operator<<(std::ostream &os, Mat<T, N, M> const &mat)
{
    os << std::endl;
    for (size_t i = 0; i < N; i++)
    {
        os << mat.row(i) << std::endl;
    }
    return os;
}

template <typename T, typename U, size_t N, size_t M>
auto operator+(Mat<T, N, M> const &a, Mat<U, N, M> const &b) -> Mat<decltype(a.data[0] + b.data[0]), N, M>
{
    Mat<decltype(a.data[0] + b.data[0]), N, M> out;
    for (size_t i = 0; i < a.size(); i++)
    {
        out.data[i] = a[i] + b[i];
    }
    return out;
}

template <typename T, typename U, size_t N, size_t M>
auto operator-(Mat<T, N, M> const &a, Mat<U, N, M> const &b) -> Mat<decltype(a.data[0] - b.data[0]), N, M>
{
    Mat<decltype(a.data[0] - b.data[0]), N, M> out;
    for (size_t i = 0; i < a.size(); i++)
    {
        out.data[i] = a[i] - b[i];
    }
    return out;
}

template <typename T, typename U, size_t N, size_t M>
auto operator*(U const scalar, Mat<T, N, M> const &a) -> Mat<decltype(scalar * a.data[0]), N, M>
{
    Mat<decltype(scalar * a.data[0]), N, M> out;
    for (size_t i = 0; i < a.size(); i++)
    {
        out.data[i] = scalar * a.data[i];
    }
    return out;
}

template <typename T, typename U, size_t N, size_t M>
auto operator*(Mat<T, N, M> const &a, U const scalar) -> Mat<decltype(a.data[0] * scalar), N, M>
{
    Mat<decltype(a.data[0] * scalar), N, M> out;
    for (size_t i = 0; i < a.size(); i++)
    {
        out.data[i] = a.data[i] * scalar;
    }
    return out;
}

template <typename T, typename U, size_t N, size_t M, size_t P>
auto operator*(Mat<T, N, M> const &a, Mat<U, M, P> const &b) -> Mat<decltype(a.data[0] * b.data[0]), N, P>
{
    Mat<decltype(a.data[0] * b.data[0]), N, P> out;
    for (size_t i = 0; i < N; i++)
    {
        for (size_t j = 0; j < P; j++)
        {
            Vec<decltype(a.data[0] * b.data[0]), M> aIthRow = a.row(i);
            Vec<decltype(a.data[0] * b.data[0]), M> bJthCol = b.col(j);
            out(i, j) = dot(aIthRow, bJthCol);
        }
    }
    return out;
}
