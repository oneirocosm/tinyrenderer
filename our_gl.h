#pragma once

#include "tgaimage.h"
#include "geometry.h"

#include <algorithm>
#include <array>

mat4x4 createLookAtMat(vec3 const &eye, vec3 const &center, vec3 const &up);
mat4x4 createPerspectiveMat(double const f);
mat4x4 createViewportMat(int const x, int const y, int const w, int const h);
std::vector<double> createZbuffer(int const width, int const height);

struct VertexOut
{
    vec4 position;
    vec3 normal;
};

struct IShader
{
    virtual std::pair<bool, TGAColor> fragment(VertexOut const &fragmentIn) const = 0;
};

void rasterize(std::array<VertexOut, 3> const &vertOut, IShader &shader, TGAImage &framebuffer, std::vector<double> &zbuffer, mat4x4 const &viewportMat);

/*
template <typename T, size_t N>
Vec<T, N> interpolate(std::array<Vec<T, N>, 3> const &surrounding, vec3 const &bc)
{
    Vec<T, N> out;
    for (size_t i = 0; i < N; i++)
    {
        out[i] = bc.x * surrounding[0] + bc.y * surrounding[1] + bc.z * surrounding[2];
    }
    return out;
}

VertexOut interpolate(std::array<VertexOut, 3> const &surrounding, vec3 const &bc, vec4 const &position)
{
    VertexOut out;
    // interpolate color
    std::array<vec3, 3> colors;
    std::transform(surrounding.begin(), surrounding.end(), colors.begin(), [&](VertexOut in)
                   { return vec4(in.normal[0], in.normal[1], in.normal[2], in.normal[3]); });
    vec3 newcolor = interpolate(colors, bc);

    // interpolate normal (if it existed)

    // overwrite position
    out.position = position;
    return out;
}

template <typename T>
TGAColor interpolate(std::array<T, 3> const &surrounding, vec3 const &bc)
{
    T out;
    for (size_t i = 0; i < 3; i++)
    {
        out[i] = bc.x * surrounding[0] + bc.y * surrounding[1] + bc.z * surrounding[2];
    }
    return out;
}
*/
