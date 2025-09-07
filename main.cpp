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
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);

    // int minX = std::min({ax, bx, cx});
    // int maxX = std::max({ax, bx, cx});
    int minY = std::min({ay, by, cy});
    int maxY = std::max({ay, by, cy});

    for (int y = minY + 1; y < maxY; y++)
    {
        double abTVal = (y - ay) / static_cast<float>(by - ay);
        int abXVal = std::round(ax * (1 - abTVal) + bx * abTVal);

        double bcTVal = (y - by) / static_cast<float>(cy - by);
        int bcXVal = std::round(bx * (1 - bcTVal) + cx * bcTVal);

        double caTVal = (y - cy) / static_cast<float>(ay - cy);
        int caXVal = std::round(cx * (1 - caTVal) + ax * caTVal);

        int leftX;
        int rightX;

        if (caTVal <= 0 || caTVal >= 1)
        {
            leftX = std::min(abXVal, bcXVal);
            rightX = std::max(abXVal, bcXVal);
            std::cout << "ca excluded" << std::endl;
        }
        else if (bcTVal <= 0 || bcTVal >= 1)
        {
            leftX = std::min(caXVal, abXVal);
            rightX = std::max(caXVal, abXVal);
            std::cout << "bc excluded" << std::endl;
        }
        else
        {
            leftX = std::min(bcXVal, caXVal);
            rightX = std::max(bcXVal, caXVal);
            std::cout << "ab excluded" << std::endl;
        }

        for (int x = leftX; x <= rightX; x++)
        {
            framebuffer.set(x, y, color);
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
