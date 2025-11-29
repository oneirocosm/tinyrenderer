#include "geometry.h"
#include "model.h"
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <regex>

Model::Model(std::vector<vec3> const vertices, std::vector<vec3> const face_normals, std::vector<int> const face_idxs) : vertices(vertices), face_normals(face_normals), face_idxs(face_idxs) {};

std::vector<std::string> Model::string_split(std::string const input, std::string const sep)
{
    std::regex split_regex(sep);
    std::vector<std::string> segments{
        std::sregex_token_iterator(input.begin(), input.end(), split_regex),
        std::sregex_token_iterator(),
    };
    return segments;
}

std::optional<double> Model::try_stod(std::string const input)
{
    char *err;
    double out = strtod(input.c_str(), &err);
    if (*err)
    {
        return std::nullopt;
    }
    return out;
}

std::optional<int> Model::try_first_idx(std::string const input)
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

std::optional<Model> Model::create(std::string const filename)
{
    std::ifstream input_stream(filename);
    if (!input_stream)
    {
        return std::nullopt;
    }
    std::vector<vec3> vertices;
    std::vector<vec3> face_normals;
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

            vec3 vertex = vec3({maybe_x.value(), maybe_y.value(), maybe_z.value()});
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
            int idx1 = maybe_idx1.value() - 1;
            int idx2 = maybe_idx2.value() - 1;
            int idx3 = maybe_idx3.value() - 1;
            face_idxs.push_back(idx1);
            face_idxs.push_back(idx2);
            face_idxs.push_back(idx3);

            // temporary get the actual positions and use them to compute face normals
            vec3 pos1 = vertices[idx1];
            vec3 pos2 = vertices[idx2];
            vec3 pos3 = vertices[idx3];
            vec3 normVec = norm(cross(pos2 - pos1, pos3 - pos1));
            face_normals.push_back(normVec);
        } // else ignore the line
    }

    return Model(vertices, face_normals, face_idxs);
}

std::optional<vec3> Model::vert(size_t const idx) const
{
    if (idx >= vertices.size())
    {
        return std::nullopt;
    }
    return vertices[idx];
};

std::optional<vec3> Model::vert(size_t const faceIdx, size_t const idx) const
{
    if (faceIdx >= nfaces() || idx > 2)
    {
        return std::nullopt;
    }
    return vertices[face_idxs[3 * faceIdx + idx]];
};

std::optional<vec3> Model::faceNormal(size_t const faceIdx, size_t const idx) const
{
    if (faceIdx >= nfaces() || idx > 2)
    {
        return std::nullopt;
    }
    return face_normals[faceIdx];
}

size_t Model::nverts() const
{
    return vertices.size();
};

size_t Model::nfaces() const
{
    return face_idxs.size() / 3;
};
