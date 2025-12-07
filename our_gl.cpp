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
        l.x, l.y, l.z, -center.x,
        m.x, m.y, m.z, -center.y,
        n.x, n.y, n.z, -center.z,
        0., 0., 0., 1.);
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
    out.normal = norm(vec3(bc[0] * vertOut[0].normal + bc[1] * vertOut[1].normal + bc[2] * vertOut[2].normal));
    out.uv = vec2(bc[0] * vertOut[0].uv + bc[1] * vertOut[1].uv + bc[2] * vertOut[2].uv);
    return out;
}

void rasterize(std::array<VertexOut, 3> const &vertOut, IShader &shader, TGAImage &framebuffer, std::vector<double> &zbuffer, mat4x4 const &viewportMat)
{
    vec4 clip[3];
    for (size_t i : {0, 1, 2})
    {
        clip[i] = vertOut[i].position;
    }

    vec4 ndc[3] = {(1 / clip[0].w) * clip[0], (1 / clip[1].w) * clip[1], (1 / clip[2].w) * clip[2]};
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
            vec3 bc = abc.invertTranspose() * vec3(static_cast<double>(x), static_cast<double>(y), 1.);
            if (bc.x < 0 || bc.y < 0 || bc.z < 0)
            {
                continue;
            }
            double z = dot(bc, vec3(ndc[0].z, ndc[1].z, ndc[2].z));
            if (z <= zbuffer[x + y * framebuffer.width()])
            {
                continue;
            }
            vec4 point(x, y, z, 1.);
            VertexOut fragIn = VertexOut::interpolate(vertOut, bc);
            fragIn.position = point;
            auto [discard, color] = shader.fragment(fragIn);
            if (discard)
            {
                continue;
            }
            framebuffer.set(x, y, color);
            zbuffer[x + y * framebuffer.width()] = z;
        }
    }
}
