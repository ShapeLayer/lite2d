#include "engine.h"

#include <iostream>
#include <cmath>
#include <cctype>
#include <initializer_list>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "debug.h"

/**
 * Initialize OpenGL state and compile shader.
 * @return True if successful, false otherwise.
 */
bool Engine::initGL()
{
  while (glGetError() != GL_NO_ERROR)
  {
  }

  const char *vs = R"(#version 330 core
        layout(location=0) in vec2 aPos;
        layout(location=1) in vec2 aUV;
        layout(location=2) in vec3 aColor;
        uniform mat4 uMVP;
        out vec2 vUV;
        out vec3 vColor;
        void main() {
            gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
            vUV = aUV;
            vColor = aColor;
        })";

  const char *fs = R"(#version 330 core
        in vec2 vUV;
        in vec3 vColor;
        uniform sampler2D uTex;
        uniform float uOpacity;
        out vec4 FragColor;
        void main() {
            vec4 tex = texture(uTex, vUV);
            FragColor = vec4(vColor, uOpacity) * tex;
        })";

  if (!shader.compile(vs, fs))
  {
    std::cerr << "Shader compilation failed\n";
    return false;
  }
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("after shader.compile");
#endif

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("after states");
#endif

  // Safe stencil query on macOS
  GLint sbits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &sbits);
  GLenum err = glGetError();
  if (err == GL_INVALID_ENUM)
  {
    sbits = 0;
    while (glGetError() != GL_NO_ERROR)
    {
    }
  }
  stencilBits = sbits;
  std::cerr << "Stencil bits: " << stencilBits << "\n";

  clearMask = GL_COLOR_BUFFER_BIT;
  if (stencilBits > 0)
    clearMask |= GL_STENCIL_BUFFER_BIT;

  return true;
}

void Engine::buildGLMeshes()
{
  for (auto &kv : model.meshes)
  {
    GLMesh gm;
    gm.create(kv.second);
    glmeshes[kv.first] = gm;
  }
}

void Engine::createCheckerTexture(const std::string &id, int w, int h)
{
  std::vector<unsigned char> pix(w * h * 4);
  for (int y = 0; y < h; y++)
  {
    for (int x = 0; x < w; x++)
    {
      int c = ((x / 8) + (y / 8)) & 1 ? 220 : 255;
      pix[(y * w + x) * 4 + 0] = (unsigned char)c;
      pix[(y * w + x) * 4 + 1] = (unsigned char)c;
      pix[(y * w + x) * 4 + 2] = (unsigned char)c;
      pix[(y * w + x) * 4 + 3] = 255;
    }
  }
  Texture t;
  t.w = w;
  t.h = h;
  glGenTextures(1, &t.id);
  glBindTexture(GL_TEXTURE_2D, t.id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  textures[id] = t;
}

// Animation sampling
void Engine::applyAnimation(const AnimationClip &clip, float t)
{
  float localT = std::fmod(t, clip.duration);
  for (const auto &tr : clip.tracks)
  {
    auto it = model.params.find(tr.param_id);
    if (it == model.params.end())
      continue;
    float v = tr.sample(localT, it->second.def_v);
    it->second.set(v);
  }
}

// Expressions
void Engine::applyExpressions(const std::vector<std::pair<std::string, float>> &exprWeights)
{
  std::unordered_map<std::string, float> add;
  std::unordered_map<std::string, std::pair<int, float>> ov;
  for (auto &ew : exprWeights)
  {
    auto it = model.expressions.find(ew.first);
    if (it == model.expressions.end())
      continue;
    float w = ew.second;
    for (auto &ep : it->second.params)
    {
      float val = ep.delta * w;
      if (ep.mode == BlendMode::Additive)
        add[ep.param_id] += val;
      else
      {
        auto cur = ov.find(ep.param_id);
        if (cur == ov.end() || ep.priority > cur->second.first)
          ov[ep.param_id] = {ep.priority, val};
      }
    }
  }
  for (auto &kv : add)
  {
    auto it = model.params.find(kv.first);
    if (it != model.params.end())
      it->second.set(it->second.cur_v + kv.second);
  }
  for (auto &kv : ov)
  {
    auto it = model.params.find(kv.first);
    if (it != model.params.end())
      it->second.set(kv.second.second);
  }
}

// Deformer world matrices (3x3 2D affine)
void Engine::computeDeformers()
{
  worldM.clear();
  std::vector<std::string> roots;
  for (auto &kv : model.deformers)
    if (kv.second.parent.empty())
      roots.push_back(kv.first);

  std::function<void(const std::string &)> dfs = [&](const std::string &id)
  {
    const auto &d = model.deformers.at(id);
    glm::mat3 local = glm::mat3(1.0f);
    float r = glm::radians(d.rot_deg);
    float c = std::cos(r), s = std::sin(r);
    glm::mat3 T = glm::mat3(1, 0, 0, 0, 1, 0, d.pos.x, d.pos.y, 1);
    glm::mat3 R = glm::mat3(c, s, 0, -s, c, 0, 0, 0, 1);
    glm::mat3 S = glm::mat3(d.scale.x, 0, 0, 0, d.scale.y, 0, 0, 0, 1);
    local = T * R * S;
    if (d.parent.empty())
      worldM[id] = local;
    else
      worldM[id] = worldM[d.parent] * local;
    for (auto &cId : d.children)
      dfs(cId);
  };
  for (auto &r : roots)
    dfs(r);
}

// CPU skinning (2 bones)
std::vector<glm::vec2> Engine::deformMesh(const ArtMesh &m) const
{
  std::vector<glm::vec2> out(m.verts.size());
  for (size_t i = 0; i < m.verts.size(); ++i)
  {
    const auto &v = m.verts[i];
    glm::vec2 p = v.pos;
    glm::vec3 hp{p.x, p.y, 1.0f};
    glm::vec3 acc{0, 0, 0};
    for (int j = 0; j < 2; j++)
    {
      int boneIdx = v.bone[j];
      float w = v.weight[j];
      if (w <= 0)
        continue;
      if (boneIdx < 0 || boneIdx >= (int)m.deformers.size())
        continue;
      const std::string &did = m.deformers[boneIdx];
      auto itM = worldM.find(did);
      if (itM == worldM.end())
        continue;
      glm::vec3 tp = itM->second * hp;
      acc += tp * w;
    }
    out[i] = glm::vec2(acc.x, acc.y);
  }
  return out;
}

glm::mat4 Engine::computeMVP(int fbw, int fbh)
{
  // aspect-fit letterbox using model canvas size; Y is already flipped in loader
  float cw = canvas.x;
  float ch = canvas.y;
  float winAspect = (float)fbw / (float)fbh;
  float canvasAspect = cw / ch;
  float sx = 1.0f, sy = 1.0f;
  if (winAspect > canvasAspect)
  {
    sx = canvasAspect / winAspect;
  }
  else
  {
    sy = winAspect / canvasAspect;
  }
  glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, 1.0f));
  proj = glm::ortho(-cw * 0.5f, cw * 0.5f, -ch * 0.5f, ch * 0.5f);
  return proj * view * S;
}

static std::string toLowerCopy(const std::string &value)
{
  std::string out = value;
  for (auto &ch : out)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return out;
}

static bool containsToken(const std::string &haystack, const std::string &needle)
{
  return haystack.find(needle) != std::string::npos;
}

static float smoothstep(float edge0, float edge1, float x)
{
  if (edge0 == edge1)
    return x >= edge1 ? 1.0f : 0.0f;
  float t = (x - edge0) / (edge1 - edge0);
  t = std::max(0.0f, std::min(1.0f, t));
  return t * t * (3.0f - 2.0f * t);
}

static float computeBlinkOpen(float timeSec)
{
  const float period = 4.0f;
  const float closeTime = 0.08f;
  const float openTime = 0.16f;
  float t = std::fmod(timeSec, period);
  if (t < closeTime)
  {
    return 1.0f - smoothstep(0.0f, closeTime, t);
  }
  if (t < openTime)
  {
    return smoothstep(closeTime, openTime, t);
  }
  return 1.0f;
}

static void applyScaleY(std::vector<glm::vec2> &verts, float scale)
{
  if (verts.empty())
    return;
  glm::vec2 minV = verts.front();
  glm::vec2 maxV = verts.front();
  for (const auto &v : verts)
  {
    minV.x = std::min(minV.x, v.x);
    minV.y = std::min(minV.y, v.y);
    maxV.x = std::max(maxV.x, v.x);
    maxV.y = std::max(maxV.y, v.y);
  }
  float centerY = (minV.y + maxV.y) * 0.5f;
  for (auto &v : verts)
  {
    v.y = centerY + (v.y - centerY) * scale;
  }
}

static void applyScaleX(std::vector<glm::vec2> &verts, float scale)
{
  if (verts.empty())
    return;
  glm::vec2 minV = verts.front();
  glm::vec2 maxV = verts.front();
  for (const auto &v : verts)
  {
    minV.x = std::min(minV.x, v.x);
    minV.y = std::min(minV.y, v.y);
    maxV.x = std::max(maxV.x, v.x);
    maxV.y = std::max(maxV.y, v.y);
  }
  float centerX = (minV.x + maxV.x) * 0.5f;
  for (auto &v : verts)
  {
    v.x = centerX + (v.x - centerX) * scale;
  }
}

static void applyTranslateY(std::vector<glm::vec2> &verts, float offset)
{
  for (auto &v : verts)
    v.y += offset;
}

void Engine::update(float timeSec, float dt)
{
  model.resetParams();
  if (!model.animations.empty())
    applyAnimation(model.animations[0], timeSec);
  // extra expressions can be applied here
  // applyExpressions({{"blink", 0.0f}});

  const float blinkOpen = computeBlinkOpen(timeSec);
  const float mouthOpenAnim = 0.2f + 0.3f * (0.5f + 0.5f * std::sin(timeSec * 1.7f));

  if (auto itEye = model.params.find("ParamEyeLOpen"); itEye != model.params.end())
    itEye->second.set(blinkOpen);
  if (auto itEye = model.params.find("ParamEyeROpen"); itEye != model.params.end())
    itEye->second.set(blinkOpen);

  if (auto itMouthY = model.params.find("ParamMouthOpenY"); itMouthY != model.params.end())
    itMouthY->second.set(mouthOpenAnim);
  else if (auto itMouth = model.params.find("ParamMouthOpen"); itMouth != model.params.end())
    itMouth->second.set(mouthOpenAnim);

  float mouthOpen = mouthOpenAnim;
  if (auto itMouthY = model.params.find("ParamMouthOpenY"); itMouthY != model.params.end())
  {
    Spring &sp = springs["ParamMouthOpenY"];
    mouthOpen = sp.update(itMouthY->second.cur_v, dt);
    itMouthY->second.set(mouthOpen);
  }
  else if (auto itMouth = model.params.find("ParamMouthOpen"); itMouth != model.params.end())
  {
    Spring &sp = springs["ParamMouthOpen"];
    mouthOpen = sp.update(itMouth->second.cur_v, dt);
    itMouth->second.set(mouthOpen);
  }

  if (auto itRoot = model.deformers.find("def_root"); itRoot != model.deformers.end())
  {
    itRoot->second.pos = {0.0f, 0.0f};
    itRoot->second.rot_deg = 0.0f;
  }

  computeDeformers();

  const bool hasFaceParts = !model.mesh_face_parts.empty();
  const float eyeLOpen = model.params.count("ParamEyeLOpen") ? model.params["ParamEyeLOpen"].cur_v : blinkOpen;
  const float eyeROpen = model.params.count("ParamEyeROpen") ? model.params["ParamEyeROpen"].cur_v : blinkOpen;
  const float eyeOpenAvg = 0.5f * (eyeLOpen + eyeROpen);
  const float mouthForm = model.params.count("ParamMouthForm") ? model.params["ParamMouthForm"].cur_v : 0.0f;
  const float browL = model.params.count("ParamBrowLY") ? model.params["ParamBrowLY"].cur_v : 0.0f;
  const float browR = model.params.count("ParamBrowRY") ? model.params["ParamBrowRY"].cur_v : 0.0f;

  auto meshHasPart = [&](const std::string &meshId, const std::initializer_list<const char *> &parts)
  {
    auto it = model.mesh_face_parts.find(meshId);
    if (it == model.mesh_face_parts.end())
      return false;
    for (const auto *part : parts)
      if (it->second.count(part))
        return true;
    return false;
  };

  for (auto &kv : model.meshes)
  {
    auto deformed = deformMesh(kv.second);

    bool isLeftEye = false;
    bool isRightEye = false;
    bool isEyeGeneric = false;
    bool isMouth = false;
    bool isBrowL = false;
    bool isBrowR = false;

    if (hasFaceParts)
    {
      isLeftEye = meshHasPart(kv.first, {"eye_left", "eyelid_left", "eye_white_left", "eye_ball_left"});
      isRightEye = meshHasPart(kv.first, {"eye_right", "eyelid_right", "eye_white_right", "eye_ball_right"});
      isEyeGeneric = meshHasPart(kv.first, {"eye"});
      isMouth = meshHasPart(kv.first, {"mouth", "lip_upper", "lip_lower", "tongue", "teeth"});
      isBrowL = meshHasPart(kv.first, {"brow_left"});
      isBrowR = meshHasPart(kv.first, {"brow_right"});
    }
    else
    {
      const std::string lowerId = toLowerCopy(kv.first);
      const bool isEye = containsToken(lowerId, "eye") && !containsToken(lowerId, "brow");
      isEyeGeneric = isEye;
      isMouth = containsToken(lowerId, "mouth") || containsToken(lowerId, "lip");
    }

    if (isLeftEye || isRightEye || isEyeGeneric)
    {
      float open = eyeOpenAvg;
      if (isLeftEye)
        open = eyeLOpen;
      else if (isRightEye)
        open = eyeROpen;
      float eyeScale = glm::mix(0.05f, 1.0f, open);
      applyScaleY(deformed, eyeScale);
    }

    if (isMouth)
    {
      float mouthScaleY = glm::mix(0.7f, 1.3f, mouthOpen);
      applyScaleY(deformed, mouthScaleY);
      float mouthScaleX = 1.0f + mouthForm * 0.2f;
      applyScaleX(deformed, mouthScaleX);
    }

    if (isBrowL || isBrowR)
    {
      float brow = isBrowL ? browL : browR;
      if (!deformed.empty())
      {
        glm::vec2 minV = deformed.front();
        glm::vec2 maxV = deformed.front();
        for (const auto &v : deformed)
        {
          minV.x = std::min(minV.x, v.x);
          minV.y = std::min(minV.y, v.y);
          maxV.x = std::max(maxV.x, v.x);
          maxV.y = std::max(maxV.y, v.y);
        }
        float height = std::max(1e-4f, maxV.y - minV.y);
        applyTranslateY(deformed, brow * height * 0.08f);
      }
    }

    glmeshes[kv.first].updatePositions(deformed);
  }
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("after update positions");
#endif
}

void Engine::render(int fbw, int fbh)
{
  glViewport(0, 0, fbw, fbh);
  glClear(clearMask); // no stencil clear if stencilBits == 0
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("after render clear");
#endif
  shader.use();
  glm::mat4 mvp = computeMVP(fbw, fbh);
  glUniformMatrix4fv(shader.loc("uMVP"), 1, GL_FALSE, &mvp[0][0]);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(shader.loc("uTex"), 0);

  std::vector<const ArtMesh *> drawList;
  drawList.reserve(model.meshes.size());
  for (auto &kv : model.meshes)
  {
    if (kv.second.visible)
      drawList.push_back(&kv.second);
  }
  std::sort(drawList.begin(), drawList.end(),
            [](auto *a, auto *b)
            { return a->draw_order < b->draw_order; });

  auto bindTex = [&](const std::string &tid)
  {
    auto it = textures.find(tid);
    if (it != textures.end())
      glBindTexture(GL_TEXTURE_2D, it->second.id);
  };

  for (auto *m : drawList)
  {
    // Set blend mode based on drawable properties
    switch (m->blend_mode)
    {
      case 1: // additive
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
      case 2: // multiply
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
      case 0: // normal
      default:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
    
    // Set opacity uniform
    glUniform1f(shader.loc("uOpacity"), m->opacity);
    
    bool canClip = (stencilBits > 0) && !m->clipping_mask_id.empty();
    if (!canClip)
    {
      bindTex(m->texture_id);
      glDisable(GL_STENCIL_TEST);
      glmeshes[m->id].draw();
    }
    else
    {
      auto itMask = model.meshes.find(m->clipping_mask_id);
      if (itMask == model.meshes.end())
        continue;
      bindTex(m->texture_id);
      glEnable(GL_STENCIL_TEST);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glStencilFunc(GL_ALWAYS, 1, 0xFF);
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
      glStencilMask(0xFF);
      glClear(GL_STENCIL_BUFFER_BIT);
      glmeshes[itMask->first].draw();

      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glStencilFunc(GL_EQUAL, 1, 0xFF);
      glStencilMask(0x00);
      glmeshes[m->id].draw();

      glDisable(GL_STENCIL_TEST);
      glStencilMask(0xFF);
    }
  }
  
  // Restore default blend mode
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
