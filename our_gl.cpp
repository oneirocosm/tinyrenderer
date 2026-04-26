#include "geometry.h"
#include "our_gl.h"
#include "tgaimage.h"

#include <algorithm>
#include <array>

mat4x4 createViewportMat(int const x, int const y, int const w, int const h)
{
    return mat4x4(
        w / 2., 0., 0., x + w / 2.,
        0., h / 2., 0., y + h / 2.,
        0., 0., 1., 0.,
        0., 0., 0., 1.);
}

mat4x4 createPerspectiveMat(double const f)
{
    return mat4x4(
        1., 0., 0., 0.,
        0., 1., 0., 0.,
        0., 0., 1., 0.,
        0., 0., -1. / f, 1.);
}

mat4x4 createLookAtMat(vec3 const &eye, vec3 const &center, vec3 const &up)
{
    vec3 n = norm(eye - center);
    vec3 l = norm(cross(up, n));
    vec3 m = norm(cross(n, l));
    return mat4x4(
               l.x, l.y, l.z, 0,
               m.x, m.y, m.z, 0,
               n.x, n.y, n.z, 0,
               0., 0., 0., 1.) *
           mat4x4(
               1, 0, 0, -center.x,
               0, 1, 0, -center.y,
               0, 0, 1, -center.z,
               0, 0, 0, 1);
}

std::vector<double> createZbuffer(int const width, int const height)
{
    std::vector<double> zbuffer(width * height, -std::numeric_limits<double>::max());
    return zbuffer;
}

VertexOut VertexOut::interpolate(std::array<VertexOut, 3> const &vertOut, vec3 const &bc)
{
    VertexOut out{};
    /*
    std::array<vec3, 3> normals;
    std::transform(vertOut.begin(), vertOut.end(), normals.begin(), [&](VertexOut in)
                   { return vec3(in.normal[0], in.normal[1], in.normal[2]); });
    out.normal = norm(interpolateInternal(normals, bc));
    */
    out.position = vec4(bc[0] * vertOut[0].position + bc[1] * vertOut[1].position + bc[2] * vertOut[2].position);
    out.fragPos = vec4(bc[0] * vertOut[0].fragPos + bc[1] * vertOut[1].fragPos + bc[2] * vertOut[2].fragPos);
    out.lightPos = vec4(bc[0] * vertOut[0].lightPos + bc[1] * vertOut[1].lightPos + bc[2] * vertOut[2].lightPos);
    out.normal = vec3(bc[0] * vertOut[0].normal + bc[1] * vertOut[1].normal + bc[2] * vertOut[2].normal);
    out.uv = vec2(bc[0] * vertOut[0].uv + bc[1] * vertOut[1].uv + bc[2] * vertOut[2].uv);
    out.tangent = vec3(bc[0] * vertOut[0].tangent + bc[1] * vertOut[1].tangent + bc[2] * vertOut[2].tangent);
    out.bitangent = vec3(bc[0] * vertOut[0].bitangent + bc[1] * vertOut[1].bitangent + bc[2] * vertOut[2].bitangent);
    return out;
}

void rasterize(std::array<VertexOut, 3> const &vertOut, IShader &shader, TGAImage &framebuffer, DepthTexture &zbuffer, mat4x4 const &viewportMat)
{
    vec4 clip[3];
    for (size_t i : {0, 1, 2})
    {
        clip[i] = vertOut[i].position;
    }

    vec4 ndc[3] = {clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w};
    vec2 screenSpace[3];
    for (size_t i : {0, 1, 2})
    {
        vec4 screen3d = viewportMat * ndc[i];
        screenSpace[i] = vec2(screen3d.x, screen3d.y);
    }

    mat3x3 abc(
        screenSpace[0].x, screenSpace[0].y, 1.,
        screenSpace[1].x, screenSpace[1].y, 1.,
        screenSpace[2].x, screenSpace[2].y, 1.);
    if (abc.det() < 1)
    {
        return;
    }

    int minX = std::min({screenSpace[0].x, screenSpace[1].x, screenSpace[2].x});
    int maxX = std::max({screenSpace[0].x, screenSpace[1].x, screenSpace[2].x});
    int minY = std::min({screenSpace[0].y, screenSpace[1].y, screenSpace[2].y});
    int maxY = std::max({screenSpace[0].y, screenSpace[1].y, screenSpace[2].y});

#pragma omp parallel for
    for (int x = std::max(minX, 0); x <= std::min(maxX, framebuffer.width() - 1); ++x)
    {
        for (int y = std::max(minY, 0); y <= std::min(maxY, framebuffer.height() - 1); ++y)
        {
            vec3 bcScreen = abc.invertTranspose() * vec3(static_cast<double>(x), static_cast<double>(y), 1.);
            vec3 bcClip(bcScreen.x / clip[0].w, bcScreen.y / clip[1].w, bcScreen.z / clip[2].w);
            bcClip = bcClip / (bcClip.x + bcClip.y + bcClip.z);
            if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0)
            {
                continue;
            }
            double z = dot(bcScreen, vec3(ndc[0].z, ndc[1].z, ndc[2].z));
            if (z <= zbuffer.get(x, y))
            {
                continue;
            }
            VertexOut fragIn = VertexOut::interpolate(vertOut, bcClip);
            auto [discard, color] = shader.fragment(fragIn);
            if (discard)
            {
                continue;
            }
            framebuffer.set(x, y, color);
            zbuffer.set(x, y, z);
        }
    }
}

DepthTexture::DepthTexture(int width, int height) : m_Width(width), m_Height(height), m_Data(std::vector<double>(width * height, -std::numeric_limits<double>::max())) {};

int DepthTexture::width() const
{
    return m_Width;
}

int DepthTexture::height() const
{
    return m_Height;
}

void DepthTexture::set(int x, int y, double depth)
{
    // todo: using std::expected would be ideal here
    assert(x >= 0);
    assert(x < m_Width);
    assert(y >= 0);
    assert(y <= m_Height);

    m_Data[y * m_Width + x] = depth;
}

double DepthTexture::get(int x, int y) const
{
    auto sampleX = std::clamp(x, 0, m_Width - 1);
    auto sampleY = std::clamp(y, 0, m_Height - 1);

    auto idx = static_cast<int>(sampleY * m_Width + sampleX);
    return m_Data[idx];
}

double DepthTexture::sample2D(double x, double y) const
{
    auto sampleX = std::clamp(x, 0., 1.);
    auto sampleY = std::clamp(y, 0., 1.);

    auto idx = static_cast<int>(std::floor(sampleY * m_Height) * m_Width + std::floor(sampleX * m_Width));
    return m_Data[idx];
}
