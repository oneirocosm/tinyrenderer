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
    DepthTexture shadowmap;
};

constexpr double PI_HALF = M_PI / 2.;

vec3 randomHemisphere(double maxRadius)
{
    double r = (static_cast<double>(rand()) / RAND_MAX) * maxRadius;
    double theta = (static_cast<double>(rand()) / RAND_MAX) * PI_HALF;
    double phi = (static_cast<double>(rand()) / RAND_MAX) * M_2_PI;

    double x = r * cos(theta) * cos(phi);
    double y = r * cos(theta) * sin(phi);
    double z = r * sin(theta);

    return vec3(x, y, z);
}

vec3 randomHemisphereSurface(double radius)
{
    double theta = acos(2 * (static_cast<double>(rand()) / RAND_MAX) - 1);
    double phi = (static_cast<double>(rand()) / RAND_MAX) * M_2_PI;

    double x = radius * cos(theta) * cos(phi);
    double y = radius * cos(theta) * sin(phi);
    double z = radius * sin(theta);

    return vec3(x, y, z);
}

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

struct PhongShader : IShader
{
    Model const &model;
    Uniforms const &uniforms;
    PhongShader(Model const &model, Uniforms const &uniforms) : model(model), uniforms(uniforms) {};
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
        vec3 ndc = lightPos.xyz() / lightPos.w;
        double x = ndc.x * 0.5 + 0.5;
        double y = ndc.y * 0.5 + 0.5;

        // auto idx = static_cast<int>(std::floor(y * shadowHeight) * shadowWidth + std::floor(x * shadowWidth));
        double closest;
        if (x < 0 || y < 0 || x > 1 || y > 1)
        // if (idx < 0 || idx > shadowWidth * shadowHeight - 1)
        {
            closest = -std::numeric_limits<double>::max();
        }
        else
        {
            closest = uniforms.shadowmap.sample2D(x, y);
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

struct BruteForceGlobalAOShader : IShader
{
    Model const &model;
    Uniforms const &uniforms;
    BruteForceGlobalAOShader(Model const &model, Uniforms const &uniforms) : model(model), uniforms(uniforms) {};
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

        vec4 normal = vec4(norm(fragmentIn.normal), 0.);

        vec4 position = fragmentIn.position;
        int x = std::floor(position.x / width);
        int y = std::floor(position.y / height);
        // int idx = std::floor(y * height) * width + std::floor(x * width);
        double closest = uniforms.shadowmap.sample2D(x, y);

        float ndotl = dot(normal, fragmentIn.lightPos);

        int shadow = 0;
        if (closest > position.z + 0.05)
        {
            shadow = 1;
        }

        TGAColor color = {255, 255, 255, 255};
        return std::make_pair(false, color);
    };
};

void saveZbuffer(
    std::string fname,
    DepthTexture const &zbuffer,
    int const width,
    int const height)
{
    TGAImage zbufferOut(width, height, TGAImage::RGB);
    auto minVal = std::numeric_limits<double>::max();
    auto maxVal = -std::numeric_limits<double>::max();
    // for (size_t i = 0; i < zbuffer.width() * zbuffer.height(); ++i)
    for (int x = 0; x < zbuffer.width(); ++x)
    {
        for (int y = 0; y < zbuffer.height(); ++y)
        {
            double value = zbuffer.get(x, y);
            if (value > -std::numeric_limits<double>::max())
            {
                minVal = std::min(minVal, value);
            }
            maxVal = std::max(maxVal, value);
        }
    }

    for (int x = 0; x < zbuffer.width(); ++x)
    {
        for (int y = 0; y < zbuffer.height(); ++y)
        {
            auto value = static_cast<unsigned char>((zbuffer.get(x, y) - minVal) / (maxVal - minVal) * 255);
            zbufferOut.set(x, y, {value, value, value});
        }
    }
    zbufferOut.write_tga_file(fname);
}

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB, sky);
    auto zbuffer = DepthTexture(width, height);

    TGAImage dummybuffer(shadowWidth, shadowHeight, TGAImage::RGB);
    auto shadowMapBuffer = DepthTexture(shadowWidth, shadowHeight);
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

    mat4x4 viewportMat = createViewportMat(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    mat4x4 perspectiveMat = createPerspectiveMat(std::sqrt(dot(eye2Center, eye2Center)));
    mat4x4 modelViewMat = createLookAtMat(eye, center, up);

    mat4x4 shadowViewportMat = createViewportMat(0, 0, shadowWidth, shadowHeight);
    mat4x4 lightPerspectiveMat = createPerspectiveMat(std::sqrt(dot(light2Center, light2Center)));
    mat4x4 lightModelViewMat = createLookAtMat(light, center, up);

    Uniforms uniforms{
        modelViewMat,
        perspectiveMat,
        lightModelViewMat,
        lightPerspectiveMat,
        shadowMapBuffer};

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
        PhongShader phongShader(model, uniforms);

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
