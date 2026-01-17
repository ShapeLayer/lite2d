#ifndef __LITE2D_TEXTURE_H__
#pragma once
#define __LITE2D_TEXTURE_H__

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ---------- Texture store ----------
class Texture {
public:
  GLuint id{0};
  int w{0}, h{0};
  Texture fromFilePath(const std::string &path);
};

#endif  // __LITE2D_TEXTURE_H__
