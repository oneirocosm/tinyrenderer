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

vec3 scale2D(vec3 input, double width, double height)
{
    return vec3((input.x + 1.0) * width / 2.0, (input.y + 1.0) * height / 2.0, 0.0);
}

vec3 scale3D(vec3 input, double width, double height)
{
    return vec3((input.x + 1.0) * width / 2.0, (input.y + 1.0) * height / 2.0, (input.z + 1.0) * 255.0 / 2.0);
}

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGB);

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
        vec3 a = scale3D(model.vert(faceIdx, 0).value(), width, height);
        vec3 b = scale3D(model.vert(faceIdx, 1).value(), width, height);
        vec3 c = scale3D(model.vert(faceIdx, 2).value(), width, height);

        TGAColor color;
        for (size_t i = 0; i < 3; i++)
        {
            color[i] = std::rand() % 255;
        }

        triangle(a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, framebuffer, zbuffer, color);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    Mat<int, 3, 4> test(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0);
    std::cout << "the data is " << test.data[0] << test.data[1] << test.data[2] << std::endl;
    std::cout << "row 2 is " << test.row(1) << " so yay!" << std::endl;
    std::cout << "col 2 is " << test.col(1) << " so yay!" << std::endl;
    std::cout << "the output is " << test << " so yay!" << std::endl;
    std::cout << "the transpose is " << test.transpose() << " so yay!" << std::endl;

    // vec2 a(1, 2);
    // vec2 b(3, 4);
    // std::cout << "the multiplication is " << dot(a, b) << " so yay!" << std::endl;

    Mat<double, 2, 3> a(1, 2, 3, 4, 5, 6);
    Mat<double, 3, 2> b(7, 8, 9, 10, 11, 12);
    std::cout << "the multiplication is " << 0.3 * a * b << " so yay!" << std::endl;

    return 0;
}
