#include "geometry.h"
#include "model.h"
#include "tgaimage.h"
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <regex>

Model::Model(
    std::vector<vec3> const vertices,
    std::vector<vec3> const face_normals,
    std::vector<Mat<double, 2, 3>> const face_tb,
    std::vector<vec2> const uvs,
    std::vector<vec3> const normals,
    std::vector<int> const positionIdxs,
    std::vector<int> const uvIdxs,
    std::vector<int> const normalIdxs,
    TGAImage const normalMap,
    TGAImage const tangentNormalMap,
    TGAImage const texDiff,
    TGAImage const texSpec) : vertices(vertices),
                              face_normals(face_normals),
                              face_tb(face_tb),
                              uvs(uvs), normals(normals),
                              positionIdxs(positionIdxs),
                              uvIdxs(uvIdxs),
                              normalIdxs(normalIdxs),
                              normalMap(normalMap),
                              tangentNormalMap(tangentNormalMap),
                              texDiff(texDiff),
                              texSpec(texSpec) {};

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
    std::cout << filename << std::endl;
    std::ifstream input_stream(filename);
    if (!input_stream)
    {
        return std::nullopt;
    }
    std::vector<vec3> vertices;
    std::vector<vec3> face_normals;
    std::vector<Mat<double, 2, 3>> face_tb;
    std::vector<vec2> uvs;
    std::vector<vec3> normals;
    std::vector<int> positionIdxs;
    std::vector<int> uvIdxs;
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
        else if (words[0] == "vn")
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
        else if (words[0] == "vt")
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

            vec2 vertex = vec2({maybe_x.value(), maybe_y.value()});
            uvs.push_back(vertex);
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

            // uvs
            std::optional<int> maybeUvIdx1 = try_idx(words[1], 1);
            std::optional<int> maybeUvIdx2 = try_idx(words[2], 1);
            std::optional<int> maybeUvIdx3 = try_idx(words[3], 1);
            if (!maybeUvIdx1.has_value() || !maybeUvIdx2.has_value() || !maybeUvIdx3.has_value())
            {
                return std::nullopt;
            }
            if (maybeUvIdx1.value() < 1 || maybeUvIdx2.value() < 1 || maybeUvIdx3.value() < 1)
            {
                return std::nullopt;
            }
            int uvIdx1 = maybeUvIdx1.value() - 1;
            int uvIdx2 = maybeUvIdx2.value() - 1;
            int uvIdx3 = maybeUvIdx3.value() - 1;
            uvIdxs.push_back(uvIdx1);
            uvIdxs.push_back(uvIdx2);
            uvIdxs.push_back(uvIdx3);

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

            // compute tangent and bitangent
            vec2 uv1 = uvs[uvIdx1];
            vec2 uv2 = uvs[uvIdx2];
            vec2 uv3 = uvs[uvIdx3];

            vec3 e0 = pos2 - pos1;
            vec3 e1 = pos3 - pos1;
            vec2 u0 = uv2 - uv1;
            vec2 u1 = uv3 - uv1;

            auto eMat = Mat<double, 2, 3>(e0.x, e0.y, e0.z, e1.x, e1.y, e1.z);
            auto uMat = mat2x2(u0.x, u0.y, u1.x, u1.y);
            auto tbMat = uMat.invertTranspose().transpose() * eMat;
            face_tb.push_back(tbMat);
        } // else ignore the line
    }

    std::regex objExt("[.]obj");
    std::string nmfile = std::regex_replace(filename, objExt, "") + "_nm.tga";
    TGAImage normalMap;
    auto ok = normalMap.read_tga_file(nmfile);
    if (!ok)
    {
        return std::nullopt;
    }

    std::string tangentnmfile = std::regex_replace(filename, objExt, "") + "_nm_tangent.tga";
    TGAImage tangentNormalMap;
    ok = tangentNormalMap.read_tga_file(tangentnmfile);
    if (!ok)
    {
        return std::nullopt;
    }

    std::string difffile = std::regex_replace(filename, objExt, "") + "_diffuse.tga";
    TGAImage texDiff;
    ok = texDiff.read_tga_file(difffile);
    if (!ok)
    {
        return std::nullopt;
    }

    std::string specfile = std::regex_replace(filename, objExt, "") + "_spec.tga";
    TGAImage texSpec;
    ok = texSpec.read_tga_file(specfile);
    if (!ok)
    {
        return std::nullopt;
    }

    return Model(
        vertices,
        face_normals,
        face_tb,
        uvs,
        normals,
        positionIdxs,
        uvIdxs,
        normalIdxs,
        normalMap,
        tangentNormalMap,
        texDiff,
        texSpec);
}

std::optional<vec3> Model::vert(size_t const idx) const
{
    if (idx >= vertices.size())
    {
        return std::nullopt;
    }
    return vertices[idx];
}

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

std::optional<Mat<double, 2, 3>> Model::faceTb(size_t const faceIdx, size_t const idx) const
{
    if (faceIdx >= nfaces() || idx > 2)
    {
        return std::nullopt;
    }
    return face_tb[faceIdx];
}

std::optional<vec2> Model::uv(size_t const faceIdx, size_t const idx) const
{
    if (faceIdx >= nfaces() || idx > 2)
    {
        return std::nullopt;
    }
    return uvs[uvIdxs[3 * faceIdx + idx]];
}

std::optional<vec3> Model::normal(size_t const faceIdx, size_t const idx) const
{
    if (faceIdx >= nfaces() || idx > 2)
    {
        return std::nullopt;
    }
    return normals[normalIdxs[3 * faceIdx + idx]];
}

vec3 Model::getNm(double const u, double const v) const
{
    int nearestU = std::round(u * normalMap.width());
    int nearestV = std::round((1 - v) * normalMap.height());
    auto sample = normalMap.get(nearestU, nearestV);
    vec3 out;
    for (size_t i : {0, 1, 2})
    {
        out.data[2 - i] = sample[i] * (2. / 255) - 1;
    }

    return norm(out);
}

vec3 Model::getTangentNm(double const u, double const v) const
{
    int nearestU = std::round(u * normalMap.width());
    int nearestV = std::round((1 - v) * normalMap.height());
    auto sample = tangentNormalMap.get(nearestU, nearestV);
    vec3 out;
    for (size_t i : {0, 1, 2})
    {
        out.data[2 - i] = sample[i] * (2. / 255) - 1;
    }

    return norm(out);
}

vec4 Model::getSpec(double const u, double const v) const
{
    int nearestU = std::round(u * normalMap.width());
    int nearestV = std::round((1 - v) * normalMap.height());
    auto sample = texSpec.get(nearestU, nearestV);
    vec4 out;
    for (size_t i : {0, 1, 2, 3})
    {
        out.data[i] = sample[i];
    }

    return out;
}

vec4 Model::getDiff(double const u, double const v) const
{
    int nearestU = std::round(u * normalMap.width());
    int nearestV = std::round((1 - v) * normalMap.height());
    auto sample = texDiff.get(nearestU, nearestV);
    vec4 out;
    for (size_t i : {0, 1, 2, 3})
    {
        out.data[i] = sample[i];
    }

    return out;
}

size_t Model::nverts() const
{
    return vertices.size();
}

size_t Model::nfaces() const
{
    return positionIdxs.size() / 3;
}
