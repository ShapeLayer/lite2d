#include <iostream>
#include <string>

#include "debug.h"
#include "shader.h"

/**
 * Compile the shader program from vertex and fragment shader sources.
 * @param vsSrc The vertex shader source code.
 * @param fsSrc The fragment shader source code.
 * @return True if compilation and linking were successful, false otherwise.
 */
bool Shader::compile(const char *vsSrc, const char *fsSrc)
{
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vsSrc, nullptr);
  glCompileShader(vs);
  checkShader(vs, "VS");
  if (!checkCompile(vs, "VERTEX"))
    return false;

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fsSrc, nullptr);
  glCompileShader(fs);
  checkShader(fs, "FS");
  if (!checkCompile(fs, "FRAGMENT"))
    return false;

  prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  checkProgram(prog, "Main");
  if (!checkLink(prog))
    return false;

  glDeleteShader(vs);
  glDeleteShader(fs);

  verifyUse();
  return true;
}

void Shader::verifyUse() const
{
  glUseProgram(0);
  if (!glIsProgram(prog))
  {
    fprintf(stderr, "prog is not a valid program object\n");
    exit(1);
  }
}

void Shader::use() const
{
  glUseProgram(prog);
  GLenum err = glGetError();
  if (err != GL_NO_ERROR)
  {
    fprintf(stderr, "Error after glUseProgram: 0x%x\n", err);
    exit(1);
  }
}
GLint Shader::loc(const char *name) const { return glGetUniformLocation(prog, name); }

bool Shader::checkCompile(GLuint sh, const char *stage)
{
  GLint ok = 0;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok)
  {
    GLint len = 0;
    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetShaderInfoLog(sh, len, nullptr, log.data());
    std::cerr << "[SHADER COMPILE FAIL][" << stage << "] " << log << "\n";
    return false;
  }
  return true;
}

bool Shader::checkLink(GLuint prg)
{
  GLint ok = 0;
  glGetProgramiv(prg, GL_LINK_STATUS, &ok);
  if (!ok)
  {
    GLint len = 0;
    glGetProgramiv(prg, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetProgramInfoLog(prg, len, nullptr, log.data());
    std::cerr << "[PROGRAM LINK FAIL] " << log << "\n";
    return false;
  }
  return true;
}
