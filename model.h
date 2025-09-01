#pragma once

#include "geometry.h"
#include <vector>
#include <string>
#include <optional>

class Model
{
    std::vector<vec3> vertices;
    std::vector<int> face_idxs;
    Model(std::vector<vec3> vertices, std::vector<int> face_idxs);
    static std::vector<std::string> string_split(std::string input, std::string sep);
    static std::optional<double> try_stod(std::string input);
    static std::optional<int> try_first_idx(std::string input);

public:
    static std::optional<Model> create(const std::string filename);
    std::vector<vec3> getVertices();
    std::vector<int> getFaceIdxs();
};