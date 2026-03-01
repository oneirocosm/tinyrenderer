#define _USE_MATH_DEFINES
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <format>
#include <numbers>
#include <string>

#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

constexpr int width = 800;
constexpr int height = 800;
constexpr int shadowWidth = 800;
constexpr int shadowHeight = 800;

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blueOld = {255, 128, 64, 255};
constexpr TGAColor blue = {255, 0, 0, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};
constexpr TGAColor black = {0, 0, 0, 255};
constexpr TGAColor sky = {229, 200, 130, 255};

struct Uniforms
{
    mat4x4 modelViewMat;
    mat4x4 perspectiveMat;
    mat4x4 lightModelViewMat;
    mat4x4 lightPerspectiveMat;
    std::vector<double> shadowmap;
};

struct ShadowMapShader : IShader
{
    Model const &model;
    Uniforms const &uniforms;
    ShadowMapShader(Model const &model, Uniforms const &uniforms) : model(model), uniforms(uniforms) {};
    VertexOut vertex(int const face, int const vert)
    {
        vec3 v = model.vert(face, vert).value();
        vec4 position = uniforms.lightModelViewMat * vec4(v.x, v.y, v.z, 1.);
        VertexOut out;
        out.position = uniforms.lightPerspectiveMat * position;
        return out;
    }
    std::pair<bool, TGAColor> fragment(VertexOut const &fragmentIn) const
    {
        TGAColor out;
        return std::make_pair(false, out);
    }
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
        out.lightPos = uniforms.lightPerspectiveMat * uniforms.lightModelViewMat * vec4(v.x, v.y, v.z, 1.);
        out.fragPos = position;
        out.position = uniforms.perspectiveMat * position;
        vec3 normal3d = model.normal(face, vert).value();
        vec4 normal(normal3d.x, normal3d.y, normal3d.z, 0.);
        out.normal = norm((uniforms.modelViewMat.invertTranspose() * normal).xyz());
        out.uv = model.uv(face, vert).value();
        out.tangent = model.faceTangent(face, vert).value();
        out.bitangent = model.faceBitangent(face, vert).value();
        return out;
    }

    std::pair<bool, TGAColor> fragment(VertexOut const &fragmentIn) const
    {
        double u = fragmentIn.uv.x;
        double v = fragmentIn.uv.y;

        vec4 lightPos = fragmentIn.lightPos;
        vec3 ndc = (1. / lightPos.w) * lightPos.xyz();
        double x = ndc.x * 0.5 + 0.5;
        double y = ndc.y * 0.5 + 0.5;

        auto idx = static_cast<int>(std::floor(y * shadowHeight) * shadowWidth + std::floor(x * shadowWidth));
        double closest;
        if (idx < 0 || idx > shadowWidth * shadowHeight - 1)
        {
            closest = -std::numeric_limits<double>::max();
        }
        else
        {
            closest = uniforms.shadowmap[idx];
        }

        vec3 tangent = norm(fragmentIn.tangent);
        vec4 tangentRot = norm(uniforms.modelViewMat * vec4(tangent, 1.));
        vec3 bitangent = norm(fragmentIn.bitangent);
        vec4 bitangentRot = norm(uniforms.modelViewMat * vec4(bitangent, 1.));

        mat4x4 darbouxFrame(tangentRot, bitangentRot, vec4(norm(fragmentIn.normal), 0), vec4(0, 0, 0, 1));
        auto normal = norm((darbouxFrame.transpose() * vec4(model.getTangentNm(u, v), 0))).xyz();

        vec4 light4d(1., 1., 1., 0);
        vec3 light = norm((uniforms.modelViewMat * light4d).xyz());
        double ambCoeff = .4;
        double diffCoeff = 1.;
        double specCoeff = 3.;

        double ambient = 1;
        double diffuse = std::max(0., dot(light, normal));

        unsigned char shadow = 0;
        if (closest > ndc.z + 0.03 * std::tan(std::acos(diffuse)))
        {
            shadow = 1;
        }
        double visibility = (1. - shadow * 0.9);

        vec3 reflected = norm(2 * normal * dot(light, normal) - light);
        double specular = (model.getSpec(u, v)[0] / 255.) * std::pow(std::max(0., reflected.z), 35.);
        double rad = std::min(1.,
                              ambCoeff * ambient +
                                  diffCoeff * visibility * diffuse +
                                  specCoeff * visibility * specular);

        auto colorVec = model.getDiff(u, v);
        TGAColor color;
        for (int i : {0, 1, 2})
        {
            color[i] = static_cast<unsigned char>(rad * colorVec[i]);
        }
        return std::make_pair(false, color);
    };
};

void saveZbuffer(
    std::string fname,
    std::vector<double> const &zbuffer,
    int const width,
    int const height)
{
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
    zbufferOut.write_tga_file(fname);
}

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB, sky);
    auto zbuffer = createZbuffer(width, height);

    TGAImage dummybuffer(shadowWidth, shadowHeight, TGAImage::RGB);
    auto shadowMapBuffer = createZbuffer(shadowWidth, shadowHeight);
    std::vector<Model> models;

    for (int i = 1; i < argc; ++i)
    {
        std::optional<Model> maybeModel = Model::create(argv[i]);
        if (!maybeModel.has_value())
        {
            std::cerr << "Error parsing file" << argv[i] << std::endl;
            return 1;
        }
        models.push_back(maybeModel.value());
    }

    vec3 const eye(-1., 0., 2.);
    vec3 const center(0., 0., 0.);
    vec3 const up(0., 1., 0.);
    vec3 const light(1., 1., 1.);

    vec3 eye2Center = eye - center;
    vec3 light2Center = light - center;

    mat4x4 viewportMat = createViewportMat(0, 0, width, height);
    mat4x4 perspectiveMat = createPerspectiveMat(std::sqrt(dot(eye2Center, eye2Center)));
    mat4x4 modelViewMat = createLookAtMat(eye, center, up);

    mat4x4 shadowViewportMat = createViewportMat(0, 0, shadowWidth, shadowHeight);
    mat4x4 lightPerspectiveMat = createPerspectiveMat(std::sqrt(dot(light2Center, light2Center)));
    mat4x4 lightModelViewMat = createLookAtMat(light, center, up);

    Uniforms uniforms{
        modelViewMat,
        perspectiveMat,
        lightModelViewMat,
        lightPerspectiveMat};

    for (auto model : models)
    {
        ShadowMapShader shadowMapShader(model, uniforms);

        for (size_t faceIdx = 0; faceIdx < model.nfaces(); faceIdx++)
        {
            std::array<VertexOut, 3> vertOut;
            for (size_t i : {0, 1, 2})
            {
                vertOut[i] = shadowMapShader.vertex(faceIdx, i);
            }

            rasterize(vertOut, shadowMapShader, dummybuffer, shadowMapBuffer, shadowViewportMat);
        }
    }
    saveZbuffer("shadow_map_buffer.tga", shadowMapBuffer, shadowWidth, shadowHeight);

    for (auto model : models)
    {
        uniforms.shadowmap = shadowMapBuffer;
        RandomShader phongShader(model, uniforms);

        for (size_t faceIdx = 0; faceIdx < model.nfaces(); faceIdx++)
        {
            std::array<VertexOut, 3> vertOut;
            for (size_t i : {0, 1, 2})
            {
                vertOut[i] = phongShader.vertex(faceIdx, i);
            }

            rasterize(vertOut, phongShader, framebuffer, zbuffer, viewportMat);
        }
    }
    framebuffer.write_tga_file("framebuffer.tga");

    saveZbuffer("zbuffer.tga", zbuffer, width, height);

    return 0;
}
