#include "ProgramObject.hpp"

#include <iostream>
#include <vector>

GLuint ProgramObject::GetLocation(const std::string &s) {
    auto it = uniform_ids.find(s);
    if (it != uniform_ids.end())
        return it->second;

    GLint unif_id = glGetUniformLocation(id, s.c_str());
    // std::cout<<"uniform "<<s<<": "<<unif_id<<"\n";

    if (unif_id == -1) {
        // std::cerr << "uniform " << s << " not found!\n";
        return -1;
    }

    uniform_ids.emplace(s, unif_id);
    return unif_id;
}

ProgramObject::ProgramObject(GLuint id) : id(id) {}

ProgramObject::ProgramObject(std::initializer_list<ShaderObject> shaders) {
    id = glCreateProgram();

    for (const ShaderObject &s : shaders) {
        glAttachShader(id, s);
    }

    glLinkProgram(id);

    GLint infoLogLength = 0, result = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &result);
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (GL_FALSE == result) {
        std::vector<char> errorMessage(infoLogLength);
        glGetProgramInfoLog(id, infoLogLength, nullptr, &errorMessage[0]);

        std::cerr << "[glLinkProgram] Shader linking error:\n"
                  << &errorMessage[0] << std::endl;
        exit(1);
    }
}

void ProgramObject::Use() { glUseProgram(id); }

void ProgramObject::Unuse() { glUseProgram(0); }

void ProgramObject::SetUniform(const std::string &name, glm::vec4 val) {
    glUniform4fv(GetLocation(name), 1, &val[0]);
}

void ProgramObject::SetUniform(const std::string &name, glm::vec3 val) {
    glUniform3fv(GetLocation(name), 1, &val[0]);
}

void ProgramObject::SetUniform(const std::string &name, glm::vec2 val) {
    glUniform2fv(GetLocation(name), 1, &val[0]);
}

void ProgramObject::SetUniform(const std::string &name, float val) {
    glUniform1f(GetLocation(name), val);
}

void ProgramObject::SetUniform(const std::string &name, double val) {
    glUniform1d(GetLocation(name), val);
}

void ProgramObject::SetUniform(const std::string &name, int val) {
    glUniform1i(GetLocation(name), val);
}

void ProgramObject::SetUniform(const std::string &name, GLuint val) {
    glUniform1i(GetLocation(name), val);
}

void ProgramObject::SetUniform(const std::string &name, glm::ivec2 val) {
    glUniform2iv(GetLocation(name), 1, &val[0]);
}

void ProgramObject::SetUniform(const std::string &name, glm::vec3 *val,
                               int count) {
    glUniform3fv(GetLocation(name), count, &(val[0][0]));
}

void ProgramObject::SetUniform(const std::string &name, glm::mat4 val) {
    glUniformMatrix4fv(GetLocation(name), 1, GL_FALSE, &val[0][0]);
}

void ProgramObject::SetUniform(const std::string& name, float* val, int count) {
    glUniform1fv(GetLocation(name), count, val);
}

void ProgramObject::SetUniform(const std::string& name, glm::vec2* val, int count) {
    glUniform2fv(GetLocation(name), count, &val[0][0]);
}

void ProgramObject::SetUniform(const std::string& name, glm::vec4* val, int count) {
    glUniform4fv(GetLocation(name), count, &val[0][0]);
}

void ProgramObject::SetTexture(const std::string &name, int id,
                               GLuint texture) {
    glActiveTexture(GL_TEXTURE0 + id);
    glBindTexture(GL_TEXTURE_2D, texture);

    SetUniform(name, id);
}

void ProgramObject::SetImage(const std::string &name, int id, GLuint texture) {
    glBindImageTexture(id, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    SetUniform(name, id);
}