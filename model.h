#pragma once

#include "geometry.h"
#include "tgaimage.h"
#include <vector>
#include <string>
#include <optional>

class Model
{
    std::vector<vec3> vertices;
    std::vector<vec3> face_normals;
    std::vector<vec2> uvs;
    std::vector<vec3> normals;
    std::vector<int> positionIdxs;
    std::vector<int> uvIdxs;
    std::vector<int> normalIdxs;
    TGAImage normalMap;
    Model(std::vector<vec3> const vertices, std::vector<vec3> const face_normals, std::vector<vec2> const uvs, std::vector<vec3> const normals, std::vector<int> const positionIdxs, std::vector<int> const uvIdxs, std::vector<int> const normalIdxs, TGAImage const normalMap);
    static std::vector<std::string> string_split(std::string const input, std::string const sep);
    static std::optional<double> try_stod(std::string const input);
    static std::optional<int> try_first_idx(std::string const input);
    static std::optional<int> try_idx(std::string const input, size_t idx);

public:
    static std::optional<Model> create(std::string const filename);
    std::optional<vec3> vert(size_t const idx) const;
    std::optional<vec3> vert(size_t const faceIdx, size_t const idx) const;
    std::optional<vec3> faceNormal(size_t const faceIdx, size_t const idx) const;
    std::optional<vec2> uv(size_t const faceIdx, size_t const idx) const;
    std::optional<vec3> normal(size_t const faceIdx, size_t const idx) const;
    vec3 getNm(double const u, double const v) const;

    size_t nverts() const;
    size_t nfaces() const;
};