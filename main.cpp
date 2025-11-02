#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <algorithm>
#include <numbers>

constexpr int width = 800;
constexpr int height = 800;

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blueOld = {255, 128, 64, 255};
constexpr TGAColor blue = {255, 0, 0, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};
constexpr TGAColor black = {0, 0, 0, 255};

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color)
{
    bool transpose = std::abs(bx - ax) < std::abs(by - ay);
    if (transpose)
    {
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx)
    {
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    int y = ay;
    int ierror = 0;
    for (int x = ax; x <= bx; x++)
    {
        if (transpose)
        {
            framebuffer.set(y, x, color);
        }
        else
        {
            framebuffer.set(x, y, color);
        }
        ierror += 2 * std::abs(by - ay);
        y += (by > ay ? 1 : -1) * (ierror > bx - ax);
        ierror -= 2 * (bx - ax) * (ierror > bx - ax);
    }
}

double triangleArea(vec2 a, vec2 b, vec2 c)
{
    return 0.5 * ((b.y - a.y) * (b.x + a.x) + (c.y - b.y) * (c.x + b.x) + (a.y - c.y) * (a.x + c.x));
}

void triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage &framebuffer, TGAImage &zbuffer, TGAColor color)
{
    int minX = std::min({ax, bx, cx});
    int maxX = std::max({ax, bx, cx});
    int minY = std::min({ay, by, cy});
    int maxY = std::max({ay, by, cy});

#pragma omp parallel for
    for (int y = minY; y <= maxY; y++)
    {
        for (int x = minX; x <= maxX; x++)
        {
            vec2 p(x, y);
            vec2 a(ax, ay);
            vec2 b(bx, by);
            vec2 c(cx, cy);
            double triArea = triangleArea(a, b, c);
            double bcpArea = triangleArea(p, b, c);
            double capArea = triangleArea(p, c, a);
            double abpArea = triangleArea(p, a, b);

            double u = bcpArea / static_cast<double>(triArea);
            double v = capArea / static_cast<double>(triArea);
            double w = abpArea / static_cast<double>(triArea);

            unsigned char depth = static_cast<unsigned char>(az * u + bz * v + cz * w);
            if ((zbuffer.get(x, y)[0] < depth) && u >= 0 && v >= 0 && w >= 0 && triArea >= 0)
            {
                framebuffer.set(x, y, color);
                zbuffer.set(x, y, {depth, depth, depth});
            }
        }
    }
}

void rasterize(vec4 const clip[3], TGAImage &framebuffer, std::vector<double> &zbufVec, TGAColor color, mat4x4 const &viewportMat)
{
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
            if (z <= zbufVec[x + y * framebuffer.width()])
            {
                continue;
            }
            framebuffer.set(x, y, color);
            zbufVec[x + y * framebuffer.width()] = z;
        }
    }
}

vec3 rot(vec3 const v)
{
    constexpr double angle = M_PI / 6.0;
    mat3x3 rotMat = mat3x3(std::cos(angle), 0., std::sin(angle), 0., 1., 0., -std::sin(angle), 0., std::cos(angle));
    return rotMat * v;
}

vec3 persp(vec3 const v)
{
    constexpr double c = 3.0;
    return 1.0 / (1 - v.z / c) * v;
}

vec3 scale2D(vec3 input)
{
    return vec3((input.x + 1.0) * width / 2.0, (input.y + 1.0) * height / 2.0, 0.0);
}

vec3 scale3D(vec3 input)
{
    return vec3((input.x + 1.0) * width / 2.0, (input.y + 1.0) * height / 2.0, (input.z + 1.41) * 255.0 / 2.82);
}

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

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(width * height, -std::numeric_limits<double>::max());

    std::optional<Model> maybeModel = Model::create(argv[1]);

    if (!maybeModel.has_value())

    {

        std::cerr << "Error parsing file" << argv[1] << std::endl;

        return 1;
    }

    Model model = maybeModel.value();
    vec3 const eye(-1., 0., 2.);
    vec3 const center(0., 0., 0.);
    vec3 const up(0., 1., 0.);

    vec3 eye2Center = eye - center;

    mat4x4 viewportMat = createViewportMat(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    mat4x4 perspectiveMat = createPerspectiveMat(std::sqrt(dot(eye2Center, eye2Center)));
    mat4x4 modelViewMat = createLookAtMat(eye, center, up);
    mat4x4 transform = perspectiveMat * modelViewMat;

    for (size_t faceIdx = 0; faceIdx < model.nfaces(); faceIdx++)

    {

        // no need to check maybe in this scenario
        vec4 a = transform * vec4(model.vert(faceIdx, 0).value(), 1.);
        vec4 b = transform * vec4(model.vert(faceIdx, 1).value(), 1.);
        vec4 c = transform * vec4(model.vert(faceIdx, 2).value(), 1.);
        vec4 triangle[3] = {a, b, c};

        TGAColor color;
        for (size_t i = 0; i < 3; i++)
        {
            color[i] = std::rand() % 255;
        }

        rasterize(triangle, framebuffer, zbuffer, color, viewportMat);
    }
    framebuffer.write_tga_file("framebuffer.tga");

    TGAImage zbufferOut(width, height, TGAImage::RGB);
    auto minVal = std::numeric_limits<double>::max();
    auto maxVal = -std::numeric_limits<double>::max();
    for (size_t i = 0; i < zbuffer.size(); ++i)
    {
        if (zbuffer[i] > -std::numeric_limits<double>::max())
        {
            minVal = std::min(minVal, zbuffer[i]);
        }
        maxVal = std::max(maxVal, zbuffer[i]);
    }

    for (size_t i = 0; i < zbuffer.size(); ++i)
    {
        auto value = static_cast<unsigned char>((zbuffer[i] - minVal) / (maxVal - minVal) * 255);
        zbufferOut.set(i % width, i / width, {value, value, value});
    }
    zbufferOut.write_tga_file("zbuffer.tga");

    return 0;
}
