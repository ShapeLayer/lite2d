#ifndef __LITE2D_MODEL_H__
#pragma once
#define __LITE2D_MODEL_H__

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

#include "anim_clip.h"
#include "expression.h"
#include "deformer.h"
#include "glmesh.h"

/**
 * A parameter that model requires.
 * @param id The unique identifier of the parameter.
 * @param min_v The minimum value of the parameter.
 * @param max_v The maximum value of the parameter.
 * @param def_v The default value of the parameter.
 * @param cur_v The current value of the parameter.
 */
struct ModelParameter
{
  std::string id;
  float min_v{-1}, max_v{1}, def_v{0}, cur_v{0};
  ModelParameter() = default;
  ModelParameter(std::string id_, float mn, float mx, float df)
      : id(std::move(id_)), min_v(mn), max_v(mx), def_v(df), cur_v(df) {}
  void reset() { cur_v = def_v; }
  void set(float v) { cur_v = glm::clamp(v, min_v, max_v); }
  float value() const { return cur_v; }
};

/**
 * The 2D model consisting of parameters, expressions, deformers, meshes, and animations.
 * @param params The model parameters.
 * @param expressions The expressions defined in the model.
 * @param deformers The deformers (bones) in the model.
 * @param meshes The 2D meshes in the model.
 * @param animations The animation clips in the model.
 */
struct Model
{
  std::unordered_map<std::string, ModelParameter> params;
  std::unordered_map<std::string, Expression> expressions;
  std::unordered_map<std::string, Deformer> deformers;
  std::unordered_map<std::string, ArtMesh> meshes;
  std::vector<AnimationClip> animations;
  void resetParams()
  {
    for (auto &kv : params)
      kv.second.reset();
  }
  void addParam(const char* id, float mn, float mx, float defv=0)
  {
    params[id] = ModelParameter{ id, mn, mx, defv };
  }
  void removeParam(const char* id)
  {
    params.erase(id);
  }
  void initParams()
  {
    addParam("ParamAngleX",-30,30);
    addParam("ParamAngleY",-30,30);
    addParam("ParamAngleZ",-30,30);
    addParam("ParamBodyAngleX",-30,30);
    addParam("ParamBodyAngleY",-30,30);
    addParam("ParamBodyAngleZ",-30,30);
    addParam("ParamEyeLOpen",0,1,1);
    addParam("ParamEyeROpen",0,1,1);
    addParam("ParamEyeBallX",-1,1,0);
    addParam("ParamEyeBallY",-1,1,0);
    addParam("ParamBrowLY",-1,1,0);
    addParam("ParamBrowRY",-1,1,0);
    addParam("ParamBrowLForm",-1,1,0);
    addParam("ParamBrowRForm",-1,1,0);
    addParam("ParamMouthOpenY",0,1,0);
    addParam("ParamMouthForm",-1,1,0);
    addParam("ParamHairFront",-1,1,0);
    addParam("ParamHairSide",-1,1,0);
    addParam("ParamHairSide2",-1,1,0);
    addParam("ParamHairBack",-1,1,0);
    addParam("ParamHairFrontFuwa",-1,1,0);
    addParam("ParamHairSideFuwa",-1,1,0);
    addParam("ParamHairBackFuwa",-1,1,0);
  }
};

#endif  // __LITE2D_MODEL_H__
