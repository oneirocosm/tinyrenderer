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
        std::fill(std::begin(crtp().x), std::begin(crtp().x) + size(), T());
    }

    T *begin()
    {
        return &crtp().x;
    }

    T *end()
    {
        return &crtp().x + size();
    }

    T const *begin() const
    {
        return &crtp().x;
    }

    T const *end() const
    {
        return &crtp().x + size();
    }

    T &operator[](size_t i)
    {
        assert(i < N);
        return *(&crtp().x + i);
    }

    T const &operator[](size_t i) const
    {
        assert(i < N);
        return *(&crtp().x + i);
    }
};

template <typename T, size_t N>
struct Vec : VecBase<T, Vec<T, N>>
{
    union
    {
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
    union
    {
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
    union
    {
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
    union
    {
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

typedef Vec<double, 2> vec2;
typedef Vec<double, 3> vec3;
typedef Vec<double, 4> vec4;
