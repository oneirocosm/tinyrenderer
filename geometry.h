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
        return &crtp().data;
    }

    T *end()
    {
        return &crtp().data + size();
    }

    T const *begin() const
    {
        return &crtp().data;
    }

    T const *end() const
    {
        return &crtp().data + size();
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
        T data[3]{};
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
