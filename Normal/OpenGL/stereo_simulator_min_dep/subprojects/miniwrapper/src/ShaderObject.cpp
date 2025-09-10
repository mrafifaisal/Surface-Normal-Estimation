#include "ShaderObject.hpp"

#include <iostream>
#include <vector>
#include <fstream>

ShaderObject::ShaderObject(const std::string &code, GLuint type) {
    this->type = type;
    id = glCreateShader(type);
    const char *str = code.c_str();
    int size = code.size();
    glShaderSource(id, 1, &str, &size);
    glCompileShader(id);

    GLint result = GL_FALSE;
    int infoLogLength;

    // forditas statuszanak lekerdezese
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (GL_FALSE == result) {
        // hibauzenet elkerese es kiirasa
        std::vector<char> VertexShaderErrorMessage(infoLogLength);
        glGetShaderInfoLog(id, infoLogLength, nullptr,
                           &VertexShaderErrorMessage[0]);

        std::cerr << "[glCompileShader] Shader compilation error in "
                  << "quad.vert"
                  << ":\n"
                  << &VertexShaderErrorMessage[0] << std::endl;
        exit(1);
    }
}

ShaderObject::~ShaderObject() { glDeleteShader(id); }

ShaderObject ShaderObject::FromFile(const std::string &path, GLuint type) {
    std::fstream vert_file(path);
    std::string code((std::istreambuf_iterator<char>(vert_file)),
                     (std::istreambuf_iterator<char>()));
    return ShaderObject(code, type);
}

ShaderObject::operator GLuint() const { return id; }
