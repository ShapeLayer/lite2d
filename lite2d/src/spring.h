#ifndef __LITE2D_SPRING_H__
#pragma once
#define __LITE2D_SPRING_H__

struct Spring
{
  float y = 0.f;
  float v = 0.f;
  float omega = 10.f; // natural frequency
  float zeta = 0.9f;  // damping ratio
  void reset(float value)
  {
    y = value;
    v = 0.f;
  }
  float update(float target, float dt)
  {
    float f = omega * omega * (target - y) - 2.f * zeta * omega * v;
    v += f * dt;
    y += v * dt;
    return y;
  }
};

#endif  // __LITE2D_SPRING_H__
