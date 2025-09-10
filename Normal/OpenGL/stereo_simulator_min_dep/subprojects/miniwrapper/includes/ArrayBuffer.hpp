#pragma once

#include <GL/glew.h>
#include <initializer_list>
#include <vector>

template <typename T> class ArrayBuffer {
  public:
    ArrayBuffer<T>(const std::vector<T> &data, GLenum usage = GL_STATIC_DRAW);
    ~ArrayBuffer<T>();

    operator GLuint();
    void Bind() const;

  private:
    GLuint id;
};

template <typename T>
ArrayBuffer<T>::ArrayBuffer(const std::vector<T> &data, GLenum usage) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(T) * data.size(), data.data(), usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template <typename T> ArrayBuffer<T>::~ArrayBuffer() {
    glDeleteBuffers(1, &id);
}

template <typename T> ArrayBuffer<T>::operator GLuint() { return id; }

template <typename T> void ArrayBuffer<T>::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
}