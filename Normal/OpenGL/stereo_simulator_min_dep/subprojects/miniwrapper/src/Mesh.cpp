#include "Mesh.hpp"
#include <fstream>
#include <unordered_map>

// for adding "/" as a delimeter
struct slash_is_space : std::ctype<char> {
    slash_is_space() : std::ctype<char>(get_table()) {}
    static mask const *get_table() {
        static mask rc[table_size];
        rc['/'] = std::ctype_base::space;
        rc['\n'] = std::ctype_base::space;
        rc[' '] = std::ctype_base::space;
        rc['\t'] = std::ctype_base::space;
        return &rc[0];
    }
};

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
inline void hash_combine(std::size_t &seed) {}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t &seed, const T &v, Rest... rest) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, rest...);
}

#define MAKE_HASHABLE(type, ...)                                               \
    namespace std {                                                            \
    template <> struct hash<type> {                                            \
        std::size_t operator()(const type &t) const {                          \
            std::size_t ret = 0;                                               \
            hash_combine(ret, __VA_ARGS__);                                    \
            return ret;                                                        \
        }                                                                      \
    };                                                                         \
    }

struct Triplet {
    size_t a, b, c;
    Triplet(size_t x, size_t y, size_t z) : a(x), b(y), c(z) {}

    bool operator==(const Triplet &other) const {
        return this->a == other.a && this->b == other.b && this->c == other.c;
    }
};
MAKE_HASHABLE(Triplet, t.a, t.b, t.c)

MeshVertex::MeshVertex(glm::vec3 p, glm::vec3 n, glm::vec3 c, glm::vec2 u)
    : pos(p), norm(n), col(c), uv(u) {}

Mesh::Mesh(std::string path, bool flip_normals) {
    std::ios_base::sync_with_stdio(false);
    std::ifstream f(path);
    f.imbue(std::locale(f.getloc(), new slash_is_space));

    std::string cur_str = "";
    while (cur_str != "v")
        f >> cur_str;

    std::vector<glm::vec3> vpos;
    std::vector<glm::vec3> vcol;
    std::vector<glm::vec3> vnorm;
    std::vector<glm::vec2> vtex;

    glm::vec3 p;
    glm::vec3 c;
    glm::vec3 default_c = {1.0f, 1.0f, 1.0f};
    glm::vec3 n;
    glm::vec3 uv;
    f >> p.x >> p.y >> p.z;

    bool has_colors = false;
    bool has_texture_coords = false;
    f >> cur_str;
    if (cur_str != "v") {
        // note: this is wrong if there is only a single vertex
        // however that file is wrong anyways
        has_colors = true;
        c.x = std::stof(cur_str);
        f >> c.y >> c.z;
        f >> cur_str;
    } else {
        c = default_c;
    }

    vpos.push_back(p);
    vcol.push_back(c);

    if (has_colors) {
        while (cur_str == "v") {
            f >> p.x >> p.y >> p.z >> c.x >> c.y >> c.z >> cur_str;
            vpos.push_back(p);
            vcol.push_back(c);
        }
    } else {
        while (cur_str == "v") {
            f >> p.x >> p.y >> p.z >> cur_str;
            vpos.push_back(p);
            vcol.push_back(default_c);
        }
    }

    // next: normals
    assert(cur_str == "vn");
    while (cur_str == "vn") {
        f >> n.x >> n.y >> n.z >> cur_str;
        vnorm.push_back(n);
        if(flip_normals)
            vnorm.back() *= -1;
    }

    // next: optional texture coordinates
    if (cur_str == "vt") {
        has_texture_coords = true;
        while (cur_str == "vt") {
            f >> uv.x >> uv.y >> cur_str;
            vtex.push_back(uv);
        }
    } else {
        vtex.push_back({0.0f, 0.0f}); // placeholder
    }

    // next: faces
    assert(cur_str == "s");
    f >> cur_str >> cur_str;
    assert(cur_str == "f");

    std::vector<MeshVertex> mesh;
    std::vector<ElementArrayBuffer<>::id_type> gpu_idx;

    while (!f.eof() && cur_str == "f") {
        std::unordered_map<Triplet, ElementArrayBuffer<>::id_type> idxmap;
        size_t vi, ti = 0, ni;

        for (int i = 0; i < 3; ++i) {
            f >> vi; --vi;
            if (has_texture_coords){
                f >> ti;
                --ti;
            }
            f >> ni; --ni;
            Triplet triple_idx = {vi, ti, ni};
            auto iter = idxmap.find(triple_idx);
            if (iter != idxmap.end())
                gpu_idx.push_back((*iter).second);
            else {
                idxmap.emplace(triple_idx, mesh.size());
                gpu_idx.push_back(mesh.size());
                mesh.emplace_back(vpos[vi], vnorm[ni], vcol[vi], vtex[ti]);
            }
        }
        f >> cur_str;
    }

    f.close();

    vbo = std::make_unique<ArrayBuffer<MeshVertex>>(mesh, GL_STATIC_DRAW);
    ib = std::make_unique<ElementArrayBuffer<>>(gpu_idx, GL_STATIC_DRAW);

    vao.AddAttribute(*vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                     offsetof(MeshVertex, pos));
    vao.AddAttribute(*vbo, 1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                     offsetof(MeshVertex, norm));
    vao.AddAttribute(*vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                     offsetof(MeshVertex, col));
    vao.AddAttribute(*vbo, 3, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                     offsetof(MeshVertex, uv));
    vao.AddIndexBuffer(*ib);

    idx_count = gpu_idx.size();
}

void Mesh::Draw() {
    vao.Bind();
    glDrawElements(GL_TRIANGLES, idx_count, ib->GetIdxType(), nullptr);
    vao.Unbind();
}