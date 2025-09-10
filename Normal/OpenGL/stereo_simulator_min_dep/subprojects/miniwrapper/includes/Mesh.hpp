#pragma once

#include "ArrayBuffer.hpp"
#include "ElementArrayBuffer.hpp"
#include "VertexArrayObject.hpp"

#include <glm/glm.hpp>
#include <memory>

struct MeshVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 col;
    glm::vec2 uv;

    MeshVertex(glm::vec3 p, glm::vec3 n, glm::vec3 c, glm::vec2 u);
};

class Mesh {
    std::unique_ptr<ArrayBuffer<MeshVertex>> vbo;
    std::unique_ptr<ElementArrayBuffer<>> ib;
    VertexArrayObject vao;
    ElementArrayBuffer<>::id_type idx_count = 0;

  public:
    Mesh(std::string filepath, bool flip_normals = false);
    void Draw();

};