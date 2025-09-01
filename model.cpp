#include "geometry.h"
#include "model.h"
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <regex>

Model::Model(std::vector<vec3> vertices, std::vector<int> face_idxs) : vertices(vertices), face_idxs(face_idxs) {};

std::vector<std::string> Model::string_split(std::string input, std::string sep)
{
    std::regex split_regex(sep);
    std::vector<std::string> segments{
        std::sregex_token_iterator(input.begin(), input.end(), split_regex),
        std::sregex_token_iterator(),
    };
    return segments;
}

std::optional<double> Model::try_stod(std::string input)
{
    char *err;
    double out = strtod(input.c_str(), &err);
    if (*err)
    {
        return std::nullopt;
    }
    return out;
}

std::optional<int> Model::try_first_idx(std::string input)
{
    std::vector<std::string> idx_str = string_split(input, "[^/]+");
    if (idx_str.size() != 3)
    {
        return std::nullopt;
    }
    char *err;
    double out = strtol(idx_str[0].c_str(), &err, 10);
    if (*err)
    {
        return std::nullopt;
    }
    return out;
}

// public

std::optional<Model> Model::create(const std::string filename)
{
    std::ifstream input_stream(filename);
    if (!input_stream)
    {
        return std::nullopt;
    }
    std::vector<vec3> vertices;
    std::vector<int> face_idxs;

    std::string line;
    while (getline(input_stream, line))
    {
        std::vector<std::string> words = string_split(line, "\\S+");
        if (words.size() == 0)
        {
            continue;
        }
        if (words[0] == "v")
        {
            if (words.size() != 4)
            {
                return std::nullopt;
            }
            std::optional<double> maybe_x = try_stod(words[1]);
            std::optional<double> maybe_y = try_stod(words[2]);
            std::optional<double> maybe_z = try_stod(words[3]);
            if (!maybe_x.has_value() || !maybe_y.has_value() || !maybe_z.has_value())
            {
                return std::nullopt;
            }

            vec3 vertex = {maybe_x.value(), maybe_y.value(), maybe_z.value()};
            vertices.push_back(vertex);
        }
        else if (words[0] == "f")
        {
            if (words.size() != 4)
            {
                return std::nullopt;
            }
            std::optional<double> maybe_idx1 = try_first_idx(words[1]);
            std::optional<double> maybe_idx2 = try_first_idx(words[2]);
            std::optional<double> maybe_idx3 = try_first_idx(words[3]);
            if (!maybe_idx1.has_value() || !maybe_idx2.has_value() || !maybe_idx3.has_value())
            {
                return std::nullopt;
            }
            if (maybe_idx1.value() < 1 || maybe_idx2.value() < 1 || maybe_idx3.value() < 1)
            {
                return std::nullopt;
            }
            face_idxs.push_back(maybe_idx1.value() - 1);
            face_idxs.push_back(maybe_idx2.value() - 1);
            face_idxs.push_back(maybe_idx3.value() - 1);
        } // else continue
    }
    return Model(vertices, face_idxs);
}

std::vector<vec3> Model::getVertices()
{
    return vertices;
}

std::vector<int> Model::getFaceIdxs()
{
    return face_idxs;
}