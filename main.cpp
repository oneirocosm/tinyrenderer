#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>

#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

constexpr int width = 800;
constexpr int height = 800;

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blueOld = {255, 128, 64, 255};
constexpr TGAColor blue = {255, 0, 0, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};
constexpr TGAColor black = {0, 0, 0, 255};

struct Uniforms
{
    mat4x4 modelViewMat;
    mat4x4 perspectiveMat;
};

struct RandomShader : IShader
{
    Model const &model;
    Uniforms const &uniforms;
    RandomShader(Model const &model, Uniforms const &uniforms) : model(model), uniforms(uniforms) {};
    VertexOut vertex(int const face, int const vert)
    {
        // auto [modelViewMat, perspectiveMat] = uniforms;
        vec3 v = model.vert(face, vert).value();
        vec4 position = uniforms.modelViewMat * vec4(v.x, v.y, v.z, 1.);
        VertexOut out;
        out.position = position;
        for (size_t i = 0; i < 3; i++)
        {
            out.color[i] = std::rand() % 255;
        }
        return out;
    }

    std::pair<bool, TGAColor> fragment(VertexOut const &fragIn) const
    {
        return std::make_pair(false, fragIn.color);
    };
};

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB);
    auto zbuffer = createZbuffer(width, height);

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

    Uniforms uniforms{
        modelViewMat,
        perspectiveMat};

    RandomShader tempShader(model, uniforms);

    for (size_t faceIdx = 0; faceIdx < model.nfaces(); faceIdx++)
    {

        // no need to check maybe in this scenario
        Triangle vertOut;
        for (size_t i : {0, 1, 2})
        {
            vertOut[i] = tempShader.vertex(faceIdx, i);
        }
        /*
        vec4 a = transform * vec4(model.vert(faceIdx, 0).value(), 1.);
        vec4 b = transform * vec4(model.vert(faceIdx, 1).value(), 1.);
        vec4 c = transform * vec4(model.vert(faceIdx, 2).value(), 1.);
        vec4 triangle[3] = {a, b, c};
        */

        rasterize(vertOut, tempShader, framebuffer, zbuffer, viewportMat);
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
