#pragma once

#include "geometry.h"
#include <vector>
#include <string>
#include <optional>

class Model
{
    std::vector<vec3> vertices;
    std::vector<int> face_idxs;
    Model(std::vector<vec3> const vertices, std::vector<int> const face_idxs);
    static std::vector<std::string> string_split(std::string const input, std::string const sep);
    static std::optional<double> try_stod(std::string const input);
    static std::optional<int> try_first_idx(std::string const input);

public:
    static std::optional<Model> create(std::string const filename);
    std::optional<vec3> vert(size_t const idx) const;
    std::optional<vec3> vert(size_t const faceIdx, size_t const idx) const;
    size_t nverts() const;
    size_t nfaces() const;
};