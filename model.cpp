#include "geometry.h"
#include "model.h"
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <regex>

Model::Model(std::vector<vec3> const vertices, std::vector<int> const face_idxs) : vertices(vertices), face_idxs(face_idxs) {};

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
            face_idxs.push_back(maybe_idx1.value() - 1);
            face_idxs.push_back(maybe_idx2.value() - 1);
            face_idxs.push_back(maybe_idx3.value() - 1);
        } // else ignore the line
    }

    std::vector<int> idxReorder(face_idxs.size() / 3);
    for (size_t i = 0; i < idxReorder.size(); i++)
    {
        idxReorder[i] = i;
    }

    std::sort(idxReorder.begin(), idxReorder.end(), [&](int const &a, int const &b)
              {
        double aminz = std::min({vertices[face_idxs[a*3 + 0]].z, vertices[face_idxs[a*3 + 1]].z, vertices[face_idxs[a*3 + 2]].z});
        double bminz = std::min({vertices[face_idxs[b*3 + 0]].z, vertices[face_idxs[b*3 + 1]].z, vertices[face_idxs[b*3 + 2]].z});

        return aminz < bminz; });

    std::vector<int> face_idxs2(face_idxs.size());
    for (size_t i = 0; i < idxReorder.size(); i++)
    {
        for (size_t j = 0; j < 3; j++)
        {
            face_idxs2[i * 3 + j] = face_idxs[idxReorder[i] * 3 + j];
        }
    }

    face_idxs = face_idxs2;

    return Model(vertices, face_idxs);
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

size_t Model::nverts() const
{
    return vertices.size();
};

size_t Model::nfaces() const
{
    return face_idxs.size() / 3;
};
