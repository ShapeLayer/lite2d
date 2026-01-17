#include "anim_clip.h"

/**
 * Samples the track at the given time.
 * @param time Time in seconds.
 * @param fallback Fallback value if there are no keyframes.
 * @return The sampled value.
 */
float Track::sample(float time, float fallback) const
{
  if (keys.empty())
    return fallback;
  if (time <= keys.front().t)
    return keys.front().v;
  if (time >= keys.back().t)
    return keys.back().v;
  int lo = 0, hi = int(keys.size()) - 1;
  while (hi - lo > 1)
  {
    int mid = (lo + hi) / 2;
    if (keys[mid].t <= time)
      lo = mid;
    else
      hi = mid;
  }
  const auto &k0 = keys[lo];
  const auto &k1 = keys[hi];
  float w = (time - k0.t) / (k1.t - k0.t);
  w = ease(k1.interp, w);
  return k0.v * (1.f - w) + k1.v * w;
}
