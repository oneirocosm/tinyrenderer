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
    vec4 fragPos;
    vec4 lightPos;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
    vec3 normal;

    static VertexOut interpolate(std::array<VertexOut, 3> const &vertOut, vec3 const &vc);
};

struct IShader
{
    virtual std::pair<bool, TGAColor> fragment(VertexOut const &fragmentIn) const = 0;
};

class DepthTexture
{
    int m_Width;
    int m_Height;
    std::vector<double> m_Data;

public:
    DepthTexture(int width, int height);

    int width() const;
    int height() const;
    void set(int x, int y, double depth);
    double get(int x, int y) const;
    double sample2D(double x, double y) const;
};

void rasterize(std::array<VertexOut, 3> const &vertOut, IShader &shader, TGAImage &framebuffer, DepthTexture &zbuffer, mat4x4 const &viewportMat);

template <typename T>
T interpolateInternal(std::array<T, 3> const &surrounding, vec3 const &bc)
{
    return bc.x * surrounding[0] + bc.y * surrounding[1] + bc.z * surrounding[2];
}
