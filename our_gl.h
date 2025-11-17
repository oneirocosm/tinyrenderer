#pragma once

#include "tgaimage.h"
#include "geometry.h"

mat4x4 createLookAtMat(vec3 const &eye, vec3 const &center, vec3 const &up);
mat4x4 createPerspectiveMat(double const f);
mat4x4 createViewportMat(int const x, int const y, int const w, int const h);
std::vector<double> createZbuffer(int const width, int const height);

struct VertexOut
{
    vec4 position;
    TGAColor color;
};

struct IShader
{
    virtual std::pair<bool, TGAColor> fragment(VertexOut const &fragIn) const = 0;
};

typedef VertexOut Triangle[3]; // a triangle primitive is made of three ordered points
void rasterize(Triangle const &vertOut, IShader &shader, TGAImage &framebuffer, std::vector<double> &zbuffer, mat4x4 const &viewportMat);
