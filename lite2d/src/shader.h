#ifndef __LITE2D_SHADER_H__
#pragma once
#define __LITE2D_SHADER_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader
{
public:
  GLuint prog = 0;
  bool compile(const char *vsSrc, const char *fsSrc);
  void verifyUse() const;
  void use() const;
  GLint loc(const char *name) const;
private:
  bool checkCompile(GLuint sh, const char *stage);
  bool checkLink(GLuint prg);
};

#endif  // __LITE2D_SHADER_H__
