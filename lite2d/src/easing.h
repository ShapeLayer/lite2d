#ifndef __LITE2D_EASING_H__
#pragma once
#define __LITE2D_EASING_H__

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

/**
 * Represents interpolation methods for easing.
 */
enum class InterpMethod
{
  Linear,
  EaseIn,
  EaseOut,
  EaseInOut
};

static float ease(InterpMethod m, float t)
{
  t = glm::clamp(t, 0.0f, 1.0f);
  switch (m)
  {
  case InterpMethod::EaseIn:
    return t * t;
  case InterpMethod::EaseOut:
    return 1.f - (1.f - t) * (1.f - t);
  case InterpMethod::EaseInOut:
    return (t < 0.5f) ? 2.f * t * t
                      : 1.f - std::pow(-2.f * t + 2.f, 2.f) / 2.f;
  case InterpMethod::Linear:
  default:
    return t;
  }
}

#endif  // __LITE2D_EASING_H__
