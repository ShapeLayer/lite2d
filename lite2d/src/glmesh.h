#ifndef __LITE2D_GLMESH_H__
#pragma once
#define __LITE2D_GLMESH_H__

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ---------- Mesh data with skinning & clipping ----------

/**
 * Represents a single vertex in an ArtMesh.
 * @param pos The position of the vertex.
 * @param uv The texture coordinates of the vertex.
 * @param color The color of the vertex.
 * @param bone The indices of up to 2 bones affecting this vertex.
 * @param weight The weights for the corresponding bones.
 */
struct Vertex
{
  glm::vec2 pos;
  glm::vec2 uv;
  glm::vec3 color;
  glm::ivec2 bone;  // up to 2 bones
  glm::vec2 weight; // weights for those bones
};

/**
 * Represents a 2D mesh in the model.
 * @param id The unique identifier of the mesh.
 * @param texture_id The ID of the texture applied to the mesh.
 * @param clipping_mask_id The ID of the clipping mask mesh (empty if none).
 * @param draw_order The draw order of the mesh.
 * @param blend_mode The blend mode (0=normal, 1=additive, 2=multiply, etc.).
 * @param opacity The opacity multiplier of the mesh.
 * @param visible Whether the mesh is visible (renderable).
 * @param verts The list of vertices in the mesh.
 * @param indices The list of indices defining the mesh triangles.
 * @param deformers The list of leaf deformers used in bone indices.
 */
struct ArtMesh
{
  std::string id;
  std::string texture_id;
  std::string clipping_mask_id; // empty if none
  int draw_order = 0;
  int blend_mode = 0;   // 0=normal, 1=additive, 2=multiply, etc.
  float opacity = 1.0f; // alpha multiplier
  bool visible = true;  // renderable visibility flag
  std::vector<Vertex> verts;
  std::vector<uint32_t> indices;
  std::vector<std::string> deformers; // leaf deformers used in bone indices
};

/**
 * Represents an OpenGL mesh for rendering.
 * @param vao The OpenGL Vertex Array Object ID.
 * @param vbo The OpenGL Vertex Buffer Object ID.
 * @param ebo The OpenGL Element Buffer Object ID.
 * @param vertCount The number of vertices in the mesh.
 * @param idxCount The number of indices in the mesh.
 * @param cpuInterleaved The CPU-side interleaved vertex data.
 */
struct GLMesh
{
  GLuint vao { 0 }, vbo { 0 }, ebo { 0 };
  size_t vertCount { 0 }, idxCount { 0 };
  std::vector<float> cpuInterleaved;
  void create(const ArtMesh &m);
  void updatePositions(const std::vector<glm::vec2> &pos);
  void draw() const;
};

#endif  // __LITE2D_GLMESH_H__
