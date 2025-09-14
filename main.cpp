#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <algorithm>

constexpr int width = 64;
constexpr int height = 64;

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

bool isInside(vec2 p, vec2 a, vec2 b, vec2 c)
{
    int triAreaDouble = triangleArea(a, b, c);
    int bcpAreaDouble = triangleArea(p, b, c);
    int capAreaDouble = triangleArea(p, c, a);
    int abpAreaDouble = triangleArea(p, a, b);

    double u = bcpAreaDouble / static_cast<double>(triAreaDouble);
    double v = capAreaDouble / static_cast<double>(triAreaDouble);
    double w = abpAreaDouble / static_cast<double>(triAreaDouble);

    return (u >= 0 && v >= 0 && w >= 0);
}

void triangle(int ax, int ay, TGAColor aColor, int bx, int by, TGAColor bColor, int cx, int cy, TGAColor cColor, TGAImage &framebuffer)
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
            int triAreaDouble = triangleArea(a, b, c);
            int bcpAreaDouble = triangleArea(p, b, c);
            int capAreaDouble = triangleArea(p, c, a);
            int abpAreaDouble = triangleArea(p, a, b);

            double u = bcpAreaDouble / static_cast<double>(triAreaDouble);
            double v = capAreaDouble / static_cast<double>(triAreaDouble);
            double w = abpAreaDouble / static_cast<double>(triAreaDouble);

            TGAColor color;
            for (size_t i = 0; i < 3; i++)
            {
                color[i] = static_cast<unsigned char>(u * aColor[i] + v * bColor[i] + w * cColor[i]);
            }

            if (u > 0 && v > 0 && w > 0 && triAreaDouble > 0)
            {
                framebuffer.set(x, y, color);
            }
            if (u > 0 && v > 0 && w > 0 && triAreaDouble > 0 && (u > 0.1 && v > 0.1 && w > 0.1))
            {
                framebuffer.set(x, y, black);
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

    int ax = 17, ay = 4;
    int bx = 55, by = 39;
    int cx = 23, cy = 59;

    triangle(ax, ay, blue, bx, by, green, cx, cy, red, framebuffer);

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
