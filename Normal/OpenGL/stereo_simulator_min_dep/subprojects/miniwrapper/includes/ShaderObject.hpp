#pragma once

#include <string>

#include <GL/glew.h>

class ShaderObject {
  public:
    ShaderObject(const std::string &code, GLuint type);
    ~ShaderObject();
    static ShaderObject FromFile(const std::string &path, GLuint type);

    operator GLuint() const;

  private:
    GLuint id;
    GLuint type;
};