#ifndef __LITE2D_ENGINE_H__
#pragma once
#define __LITE2D_ENGINE_H__

#include <unordered_map>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "anim_clip.h"
#include "deformer.h"
#include "expression.h"
#include "model.h"
#include "glmesh.h"
#include "shader.h"
#include "texture.h"
#include "easing.h"
#include "spring.h"

/**
 * The main 2D engine class that handles model, rendering, and animation.
 * @param model The 2D model.
 * @param stencilBits Number of bits in the stencil buffer.
 * @param clearMask The OpenGL clear mask for the framebuffer.
 * @param worldM The world matrices for deformers.
 * @param glmeshes The OpenGL meshes for rendering.
 * @param textures The loaded textures.
 * @param shader The shader program used for rendering.
 * @param proj The projection matrix.
 * @param view The view matrix.
 * @param canvas The canvas size.
 * @param springs The parameter smoothing springs.
 */
class Engine
{
public:
  Model model;

  int stencilBits = 0;
  GLbitfield clearMask = GL_COLOR_BUFFER_BIT;

  std::unordered_map<std::string, glm::mat3> worldM; // deformer world matrix
  std::unordered_map<std::string, GLMesh> glmeshes;
  std::unordered_map<std::string, Texture> textures;

  // render state
  Shader shader;
  glm::mat4 proj;
  glm::mat4 view = glm::mat4(1.0f);
  glm::vec2 canvas{1920, 1080};

  // param smoothing
  std::unordered_map<std::string, Spring> springs;

  // When false, skip internal animation/reset so external code can drive params.
  bool autoAnimate = true;

  // Face pose amplification and translation scale (for ParamAngleX/Y/Z).
  float faceAngleAmplify = 1.5f;
  float facePosScale = 0.02f;

  bool initGL();
  void buildGLMeshes();
  void createCheckerTexture(const std::string &id, int w = 64, int h = 64);
  
  // Animation sampling
  void applyAnimation(const AnimationClip &clip, float t);

  // Expressions
  void applyExpressions(const std::vector<std::pair<std::string, float>> &exprWeights);

  // Deformer world matrices (3x3 2D affine)
  void computeDeformers();

  // CPU skinning (2 bones)
  std::vector<glm::vec2> deformMesh(const ArtMesh &m) const;

  glm::mat4 computeMVP(int fbw, int fbh);

  void update(float timeSec, float dt);

  void render(int fbw, int fbh);
};

#endif  // __LITE2D_ENGINE_H__
