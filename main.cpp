#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <algorithm>

constexpr int width = 128;
constexpr int height = 128;

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

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color)
{
    if (ay > by)
    {
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    if (ay > cy)
    {
        std::swap(ax, cx);
        std::swap(ay, cy);
    }
    if (by > cy)
    {
        std::swap(bx, cx);
        std::swap(by, cy);
    }
    int totalHeight = cy - ay;

    // check for degenerate case of bottom half
    if (ay != by)
    {
        int segmentHeight = by - ay;
        for (int y = ay; y <= by; y++)
        {
            int x1 = ax + ((cx - ax) * (y - ay)) / totalHeight;
            int x2 = ax + ((bx - ax) * (y - ay)) / segmentHeight;
            for (int x = std::min(x1, x2); x < std::max(x1, x2); x++)
            {
                framebuffer.set(x, y, color);
            }
        }
    }
    // check for degenerate case of upper half
    if (by != cy)
    {
        int segmentHeight = cy - by;
        for (int y = by; y <= cy; y++)
        {
            int x1 = ax + ((cx - ax) * (y - ay)) / totalHeight;
            int x2 = bx + ((cx - bx) * (y - by)) / segmentHeight;
            for (int x = std::min(x1, x2); x < std::max(x1, x2); x++)
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

    triangle(7, 45, 35, 100, 45, 60, framebuffer, red);
    triangle(120, 35, 90, 5, 45, 110, framebuffer, white);
    triangle(115, 83, 80, 90, 85, 120, framebuffer, green);

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
