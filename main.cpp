#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <algorithm>

constexpr int width = 800;
constexpr int height = 800;

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

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

bool isInside(vec2 p, vec2 a, vec2 b, vec2 c)
{
    int triAreaDouble = (b - a).wedgeComp(c - b);
    int abpAreaDouble = (a - p).wedgeComp(b - a);
    int bcpAreaDouble = (b - p).wedgeComp(c - b);
    int capAreaDouble = (c - p).wedgeComp(a - c);

    double u = bcpAreaDouble / static_cast<double>(triAreaDouble);
    double v = capAreaDouble / static_cast<double>(triAreaDouble);
    double w = abpAreaDouble / static_cast<double>(triAreaDouble);

    return (u >= 0 && v >= 0 && w >= 0);
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color)
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
            vec2 a(ax, ay);
            vec2 b(bx, by);
            vec2 c(cx, cy);
            int triAreaDouble = (b - a).wedgeComp(c - b);
            if (isInside(vec2(x, y), vec2(ax, ay), vec2(bx, by), vec2(cx, cy)) && triAreaDouble > 0)
            {
                framebuffer.set(x, y, color);
            }
        }
    }
}

vec3 scale2D(vec3 input, double width, double height)
{
    return vec3((input.x + 1.0) * width / 2.0, (input.y + 1.0) * height / 2.0, 0.0);
}

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB);

    std::optional<Model> maybeModel = Model::create(argv[1]);
    if (!maybeModel.has_value())
    {
        std::cerr << "Error parsing file" << argv[1] << std::endl;
        return 1;
    }
    Model model = maybeModel.value();

    for (size_t faceIdx = 0; faceIdx < model.nfaces(); faceIdx++)
    {
        // no need to check maybe in this scenario
        vec3 a = scale2D(model.vert(faceIdx, 0).value(), width, height);
        vec3 b = scale2D(model.vert(faceIdx, 1).value(), width, height);
        vec3 c = scale2D(model.vert(faceIdx, 2).value(), width, height);

        TGAColor color;
        for (size_t i = 0; i < 3; i++)
        {
            color[i] = std::rand() % 255;
        }

        triangle(a.x, a.y, b.x, b.y, c.x, c.y, framebuffer, color);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
