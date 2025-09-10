#include "VertexArrayObject.hpp"

VertexArrayObject::VertexArrayObject() {
    glGenVertexArrays(1, &id);
}

VertexArrayObject::~VertexArrayObject() {
    glDeleteVertexArrays(1, &id);
}

void VertexArrayObject::Bind() const {
    glBindVertexArray(id);
}

void VertexArrayObject::Unbind() const {
    glBindVertexArray(GL_NONE);
}