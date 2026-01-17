#ifndef __LITE2D_DEBUG_H__
#pragma once
#define __LITE2D_DEBUG_H__

#define LITE2D_DEBUG 1

#include <iostream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

static void checkShader(GLuint s, const char *name)
{
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok)
  {
    GLint len = 0;
    glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetShaderInfoLog(s, len, nullptr, log.data());
    fprintf(stderr, "Shader %s compile failed:\n%s\n", name, log.c_str());
    exit(1);
  }
};

static void checkProgram(GLuint p, const char *name)
{
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok)
  {
    GLint len = 0;
    glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetProgramInfoLog(p, len, nullptr, log.data());
    fprintf(stderr, "Program %s link failed:\n%s\n", name, log.c_str());
    exit(1);
  }
};

static void checkErr(const char* where) {
    bool any = false;
    for (GLenum err; (err = glGetError()) != GL_NO_ERROR; ) {
        any = true;
        std::cerr << "[GL ERROR] 0x" << std::hex << err << std::dec
                  << " at " << where << "\n";
    }
}

#endif  // __LITE2D_DEBUG_H__
