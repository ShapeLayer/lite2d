#ifndef __LITE2D_DEFORMER_H__
#pragma once
#define __LITE2D_DEFORMER_H__

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

/**
 * Represents a deformer (bone) in the model.
 * @param id The unique identifier of the deformer.
 * @param parent The ID of the parent deformer (empty if root).
 * @param pos The local position of the deformer.
 * @param rot_deg The local rotation of the deformer in degrees.
 * @param scale The local scale of the deformer.
 * @param children The IDs of the child deformers.
 * @param bound_meshes The IDs of the meshes bound to this deformer.
 */
struct Deformer {
  std::string id;
  std::string parent; // empty => root
  glm::vec2 pos { 0, 0 };
  float rot_deg { 0 };
  glm::vec2 scale { 1, 1 };
  std::vector<std::string> children;
  std::vector<std::string> bound_meshes;
};

#endif  // __LITE2D_DEFORMER_H__
