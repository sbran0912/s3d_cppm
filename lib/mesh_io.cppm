module;

#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

export module mesh_io;

import mesh;
import math_core;

export Mesh LoadOBJ(const std::string& path);

namespace {

std::string Trim(const std::string& s) {
    size_t b = 0;
    size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    return s.substr(b, e - b);
}

std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

int ObjIndex(const std::string& tok, int vertexCount, int lineNo) {
    std::string s = tok;
    const size_t slash = s.find('/');
    if (slash != std::string::npos) s = s.substr(0, slash);
    if (s.empty()) {
        throw std::runtime_error("OBJ Zeile " + std::to_string(lineNo) +
                                 ": ungultiger Index \"" + tok + "\"");
    }
    long n;
    try {
        n = std::stol(s);
    } catch (const std::exception&) {
        throw std::runtime_error("OBJ Zeile " + std::to_string(lineNo) +
                                 ": ungultiger Index \"" + tok + "\"");
    }
    if (n < 0) {
        n = vertexCount + n;
    } else {
        n = n - 1;
    }
    if (n < 0 || n >= vertexCount) {
        throw std::runtime_error("OBJ Zeile " + std::to_string(lineNo) +
                                 ": Index \"" + tok + "\" ausserhalb des Bereichs");
    }
    return static_cast<int>(n);
}

Mesh ParseOBJ(const std::string& data) {
    std::vector<Vec3> verts;
    Mesh mesh;

    std::istringstream iss(data);
    std::string raw;
    int lineNo = 0;
    while (std::getline(iss, raw)) {
        lineNo++;
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
        const std::string line = Trim(raw);
        if (line.empty() || line[0] == '#') continue;

        const std::vector<std::string> fields = Fields(line);
        if (fields.empty()) continue;
        const int vertexCount = static_cast<int>(verts.size());

        if (fields[0] == "v") {
            if (fields.size() < 4) {
                throw std::runtime_error("OBJ Zeile " + std::to_string(lineNo) +
                                         ": unvollstaendiger Vertex");
            }
            double x, y, z;
            try {
                x = std::stod(fields[1]);
                y = std::stod(fields[2]);
                z = std::stod(fields[3]);
            } catch (const std::exception&) {
                throw std::runtime_error("OBJ Zeile " + std::to_string(lineNo) +
                                         ": ungueltiger Vertex");
            }
            verts.push_back(Vec3{x, y, z});

        } else if (fields[0] == "f") {
            std::vector<int> idx;
            idx.reserve(fields.size() - 1);
            for (size_t k = 1; k < fields.size(); k++) {
                idx.push_back(ObjIndex(fields[k], vertexCount, lineNo));
            }
            for (size_t j = 1; j + 1 < idx.size(); j++) {
                const Vec3& a = verts[idx[0]];
                const Vec3& b = verts[idx[j]];
                const Vec3& c = verts[idx[j + 1]];
                mesh.Triangles.insert(mesh.Triangles.end(),
                    {static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z),
                     static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z),
                     static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z)});
            }

        } else if (fields[0] == "l") {
            std::vector<int> idx;
            idx.reserve(fields.size() - 1);
            for (size_t k = 1; k < fields.size(); k++) {
                idx.push_back(ObjIndex(fields[k], vertexCount, lineNo));
            }
            for (size_t j = 0; j + 1 < idx.size(); j++) {
                const Vec3& a = verts[idx[j]];
                const Vec3& b = verts[idx[j + 1]];
                mesh.Lines.insert(mesh.Lines.end(),
                    {static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z),
                     static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z)});
            }
        }
    }
    return mesh;
}

} // namespace

export Mesh LoadOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("OBJ-Datei nicht lesbar: " + path);
    }
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return ParseOBJ(data);
}
