#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

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

vec3 scale2D(vec3 input, double xScale, double yScale)
{
    return vec3{(input.x + 1.0) * xScale / 2.0, (input.y + 1.0) * yScale / 2.0, 0};
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Incorrect number or arguments. Must be called with 1 argument!" << std::endl;
        return 1;
    }

    constexpr int width = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    std::optional<Model> maybeModel = Model::create(argv[1]);
    if (!maybeModel.has_value())
    {
        std::cerr << "Error parsing file" << argv[1] << std::endl;
        return 1;
    }
    Model model = maybeModel.value();

    std::vector<vec3> vertices = model.getVertices();
    std::vector<int> faceIdxs = model.getFaceIdxs();
    for (size_t i = 0; i + 2 < faceIdxs.size(); i += 3)
    {
        vec3 a = scale2D(vertices[faceIdxs[i]], width, height);
        vec3 b = scale2D(vertices[faceIdxs[i + 1]], width, height);
        vec3 c = scale2D(vertices[faceIdxs[i + 2]], width, height);
        line(a.x, a.y, b.x, b.y, framebuffer, red);
        line(b.x, b.y, c.x, c.y, framebuffer, red);
        line(c.x, c.y, a.x, a.y, framebuffer, red);
    }

    for (vec3 vertex : vertices)
    {
        vec3 scaled = scale2D(vertex, width, height);
        framebuffer.set(scaled.x, scaled.y, white);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
