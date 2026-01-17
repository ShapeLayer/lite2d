#ifndef __LITE2D_ANIMATION_CLIP_H__
#pragma once
#define __LITE2D_ANIMATION_CLIP_H__

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "easing.h"

/**
 * Represents a keyframe that used in animation track.
 * @param t Time in seconds.
 * @param v Value at the keyframe.
 * @param interp Interpolation method to the next keyframe.
 */
struct Keyframe
{
  float t; // seconds
  float v;
  InterpMethod interp { InterpMethod::Linear };
};

/**
 * Represents an animation track that animates a single parameter.
 * @param param_id The ID of the parameter to animate.
 * @param keys The keyframes in the track.
 */
struct Track
{
  std::string param_id;
  std::vector<Keyframe> keys;

  float sample(float time, float fallback) const;
};

/**
 * Represents an animation clip consisting of multiple tracks.
 * @param name The name of the animation clip.
 * @param duration The duration of the animation clip in seconds.
 * @param tracks The animation tracks in the clip.
 */
struct AnimationClip
{
  std::string name;
  float duration { 1.f };
  std::vector<Track> tracks;
};

#endif  // __LITE2D_ANIMATION_CLIP_H__
