#include "geometry.h"
#include "model.h"
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <regex>

Model::Model(std::vector<vec3> const vertices, std::vector<vec3> const face_normals, std::vector<vec3> const normals, std::vector<int> const positionIdxs, std::vector<int> const normalIdxs) : vertices(vertices), face_normals(face_normals), normals(normals), positionIdxs(positionIdxs), normalIdxs(normalIdxs) {};

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

std::optional<int> Model::try_idx(std::string const input, size_t idx)
{
    std::vector<std::string> idx_str = string_split(input, "[^/]+");
    if (idx > 2)
    {
        return std::nullopt;
    }
    if (idx_str.size() != 3)
    {
        return std::nullopt;
    }
    char *err;
    double out = strtol(idx_str[idx].c_str(), &err, 10);
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
    std::vector<vec3> normals;
    std::vector<int> positionIdxs;
    std::vector<int> normalIdxs;

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
        if (words[0] == "vn")
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
            normals.push_back(norm(vertex));
        }
        else if (words[0] == "f")
        {
            if (words.size() != 4)
            {
                return std::nullopt;
            }

            // positions
            std::optional<int> maybe_idx1 = try_first_idx(words[1]);
            std::optional<int> maybe_idx2 = try_first_idx(words[2]);
            std::optional<int> maybe_idx3 = try_first_idx(words[3]);
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
            positionIdxs.push_back(idx1);
            positionIdxs.push_back(idx2);
            positionIdxs.push_back(idx3);

            // normals
            std::optional<int> maybeNormalIdx1 = try_idx(words[1], 2);
            std::optional<int> maybeNormalIdx2 = try_idx(words[2], 2);
            std::optional<int> maybeNormalIdx3 = try_idx(words[3], 2);
            if (!maybeNormalIdx1.has_value() || !maybeNormalIdx2.has_value() || !maybeNormalIdx3.has_value())
            {
                return std::nullopt;
            }
            if (maybeNormalIdx1.value() < 1 || maybeNormalIdx2.value() < 1 || maybeNormalIdx3.value() < 1)
            {
                return std::nullopt;
            }
            int normalIdx1 = maybeNormalIdx1.value() - 1;
            int normalIdx2 = maybeNormalIdx2.value() - 1;
            int normalIdx3 = maybeNormalIdx3.value() - 1;
            normalIdxs.push_back(normalIdx1);
            normalIdxs.push_back(normalIdx2);
            normalIdxs.push_back(normalIdx3);

            // compute face normals
            vec3 pos1 = vertices[idx1];
            vec3 pos2 = vertices[idx2];
            vec3 pos3 = vertices[idx3];
            vec3 normVec = norm(cross(pos2 - pos1, pos3 - pos1));
            face_normals.push_back(normVec);
        } // else ignore the line
    }

    return Model(vertices, face_normals, normals, positionIdxs, normalIdxs);
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
    return vertices[positionIdxs[3 * faceIdx + idx]];
};

std::optional<vec3> Model::faceNormal(size_t const faceIdx, size_t const idx) const
{
    if (faceIdx >= nfaces() || idx > 2)
    {
        return std::nullopt;
    }
    return face_normals[faceIdx];
}

std::optional<vec3> Model::normal(size_t const faceIdx, size_t const idx) const
{
    if (faceIdx >= nfaces() || idx > 2)
    {
        return std::nullopt;
    }
    return normals[normalIdxs[3 * faceIdx + idx]];
};

size_t Model::nverts() const
{
    return vertices.size();
};

size_t Model::nfaces() const
{
    return positionIdxs.size() / 3;
};
