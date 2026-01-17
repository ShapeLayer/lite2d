#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "debug.h"
#include "deformer.h"
#include "easing.h"
#include "engine.h"
#include "expression.h"
#include "model.h"
#include "glmesh.h"
#include "shader.h"
#include "texture.h"
#include "model_loader.h"

static void APIENTRY glDebugCb(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei,
                               const GLchar *message, const void *)
{
  std::cerr << "[GL DEBUG] id=" << id
            << " src=0x" << std::hex << source
            << " type=0x" << type
            << " sev=0x" << severity << std::dec
            << " msg=" << message << "\n";
}

void initDebug()
{
#if defined(GLAD_GL_KHR_debug)
  if (GLAD_GL_KHR_debug)
  {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCb, nullptr);
  }
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(glDebugCb, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
                        0, nullptr, GL_TRUE);
#elif defined(GLAD_GL_ARB_debug_output)
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
  glDebugMessageCallbackARB(glDebugCb, nullptr);
  glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
                           0, nullptr, GL_TRUE);
#else
  // Debug output extension not present in current GLAD build.
#endif
}


// ---------- Sample model creation ----------
static void makeSampleModel(Model &model)
{
  // Params
  model.initParams();

  // Animation: blink + idle mouth
  {
    AnimationClip clip;
    clip.name = "idle";
    clip.duration = 3.0f;
    Track eye;
    eye.param_id = "ParamEyeLOpen";
    eye.keys = {
        {0.0f, 1.0f, InterpMethod::EaseInOut},
        {0.15f, 0.0f, InterpMethod::EaseInOut},
        {0.3f, 1.0f, InterpMethod::EaseInOut},
        {2.5f, 1.0f, InterpMethod::Linear},
        {3.0f, 1.0f, InterpMethod::Linear},
    };
    Track mouth;
    mouth.param_id = "ParamMouthOpen";
    mouth.keys = {
        {0.0f, 0.1f, InterpMethod::EaseInOut},
        {1.0f, 0.5f, InterpMethod::EaseInOut},
        {2.0f, 0.15f, InterpMethod::EaseInOut},
        {3.0f, 0.1f, InterpMethod::Linear},
    };
    clip.tracks.push_back(eye);
    clip.tracks.push_back(mouth);
    model.animations.push_back(clip);
  }

  // Expression: blink override (not used by default)
  {
    Expression blink;
    blink.name = "blink";
    blink.params.push_back({"ParamEyeLOpen", 0.0f, BlendMode::Override, 10});
    model.expressions.emplace(blink.name, blink);
  }

  // Deformers
  {
    Deformer face;
    face.id = "def_face";
    face.parent = "";
    face.pos = {0, 0};
    model.deformers.emplace(face.id, face);

    Deformer eye;
    eye.id = "def_left_eye";
    eye.parent = "def_face";
    eye.pos = {-0.25f, 0.15f};
    model.deformers.emplace(eye.id, eye);

    Deformer jaw;
    jaw.id = "def_jaw";
    jaw.parent = "def_face";
    jaw.pos = {0.0f, -0.05f};
    model.deformers.emplace(jaw.id, jaw);

    model.deformers["def_face"].children.push_back("def_left_eye");
    model.deformers["def_face"].children.push_back("def_jaw");
  }

  auto quad = [](float w, float h, glm::vec3 color)
  {
    std::vector<Vertex> v(4);
    v[0].pos = {-w, -h};
    v[1].pos = {w, -h};
    v[2].pos = {w, h};
    v[3].pos = {-w, h};
    v[0].uv = {0, 0};
    v[1].uv = {1, 0};
    v[2].uv = {1, 1};
    v[3].uv = {0, 1};
    for (int i = 0; i < 4; i++)
    {
      v[i].color = color;
      v[i].bone = {0, 0};
      v[i].weight = {1, 0};
    }
    std::vector<uint32_t> idx = {0, 1, 2, 0, 2, 3};
    return std::make_pair(v, idx);
  };

  // Face mesh (root)
  {
    auto [v, idx] = quad(0.7f, 0.7f, {0.95f, 0.8f, 0.72f});
    ArtMesh m;
    m.id = "mesh_face";
    m.texture_id = "tex_checker";
    m.draw_order = 0;
    m.deformers = {"def_face"};
    m.verts = v;
    m.indices = idx;
    model.meshes.emplace(m.id, m);
    model.deformers["def_face"].bound_meshes.push_back(m.id);
  }

  // Eye white (mask)
  {
    auto [v, idx] = quad(0.12f, 0.06f, {0.95f, 0.95f, 1.0f});
    ArtMesh m;
    m.id = "mask_left_eye";
    m.texture_id = "tex_checker";
    m.draw_order = 5;
    m.deformers = {"def_left_eye"};
    // bone index 0 refers to deformers[0] in m.deformers
    for (auto &vert : m.verts)
    {
      vert.bone = {0, 0};
      vert.weight = {1, 0};
    }
    m.verts = v;
    m.indices = idx;
    model.meshes.emplace(m.id, m);
    model.deformers["def_left_eye"].bound_meshes.push_back(m.id);
  }

  // Iris (clipped by mask)
  {
    auto [v, idx] = quad(0.06f, 0.06f, {0.2f, 0.4f, 0.9f});
    ArtMesh m;
    m.id = "mesh_left_iris";
    m.texture_id = "tex_checker";
    m.draw_order = 6;
    m.clipping_mask_id = "mask_left_eye";
    m.deformers = {"def_left_eye"};
    for (auto &vert : m.verts)
    {
      vert.bone = {0, 0};
      vert.weight = {1, 0};
    }
    m.verts = v;
    m.indices = idx;
    model.meshes.emplace(m.id, m);
    model.deformers["def_left_eye"].bound_meshes.push_back(m.id);
  }

  // Mouth (bound to jaw)
  {
    auto [v, idx] = quad(0.18f, 0.05f, {0.8f, 0.2f, 0.2f});
    ArtMesh m;
    m.id = "mesh_mouth";
    m.texture_id = "tex_checker";
    m.draw_order = 7;
    m.deformers = {"def_jaw"};
    for (auto &vert : m.verts)
    {
      vert.bone = {0, 0};
      vert.weight = {1, 0};
    }
    model.meshes.emplace(m.id, m);
    model.deformers["def_jaw"].bound_meshes.push_back(m.id);
  }
}

// ---------- GLFW error callback ----------
static void glfwError(int code, const char *desc)
{
  std::cerr << "[GLFW ERROR] (" << code << ") " << desc << "\n";
}

// ---------- Main ----------
struct ViewerState
{
  Engine *engine = nullptr;
  float zoom = 1.25f;
  glm::vec2 pan{0.0f, 0.0f};
  bool panning = false;
  double lastX = 0.0;
  double lastY = 0.0;
  int fbw = 1;
  int fbh = 1;
};

static float clampFloat(float v, float lo, float hi)
{
  return std::max(lo, std::min(hi, v));
}

static float canvasPixelsPerUnit(const ViewerState &state)
{
  if (!state.engine)
    return 1.0f;
  const float cw = state.engine->canvas.x;
  const float ch = state.engine->canvas.y;
  const float scaleX = state.fbw / std::max(1.0f, cw);
  const float scaleY = state.fbh / std::max(1.0f, ch);
  return std::min(scaleX, scaleY);
}

static void scrollCallback(GLFWwindow *window, double, double yoffset)
{
  auto *state = static_cast<ViewerState *>(glfwGetWindowUserPointer(window));
  if (!state)
    return;
  const float zoomFactor = std::pow(1.15f, static_cast<float>(yoffset));
  const float prevZoom = state->zoom;
  const float nextZoom = clampFloat(state->zoom * zoomFactor, 0.5f, 8.0f);
  if (nextZoom == prevZoom)
    return;

  double cursorX = 0.0;
  double cursorY = 0.0;
  glfwGetCursorPos(window, &cursorX, &cursorY);

  const float scale = canvasPixelsPerUnit(*state);
  if (scale <= 0.0f)
    return;

  const float halfW = state->fbw * 0.5f;
  const float halfH = state->fbh * 0.5f;
  const float viewX = static_cast<float>(cursorX) - halfW;
  const float viewY = halfH - static_cast<float>(cursorY);

  const float worldX = viewX / (scale * prevZoom) - state->pan.x;
  const float worldY = viewY / (scale * prevZoom) - state->pan.y;

  state->zoom = nextZoom;
  state->pan.x = viewX / (scale * nextZoom) - worldX;
  state->pan.y = viewY / (scale * nextZoom) - worldY;
}

static void mouseButtonCallback(GLFWwindow *window, int button, int action, int)
{
  auto *state = static_cast<ViewerState *>(glfwGetWindowUserPointer(window));
  if (!state)
    return;
  if (button == GLFW_MOUSE_BUTTON_MIDDLE)
  {
    if (action == GLFW_PRESS)
    {
      state->panning = true;
      glfwGetCursorPos(window, &state->lastX, &state->lastY);
    }
    else if (action == GLFW_RELEASE)
    {
      state->panning = false;
    }
  }
}

static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
  auto *state = static_cast<ViewerState *>(glfwGetWindowUserPointer(window));
  if (!state || !state->panning)
    return;
  const double dx = xpos - state->lastX;
  const double dy = ypos - state->lastY;
  state->lastX = xpos;
  state->lastY = ypos;
  const float scale = canvasPixelsPerUnit(*state);
  if (scale <= 0.0f)
    return;
  state->pan.x += static_cast<float>(dx) / (scale * state->zoom);
  state->pan.y -= static_cast<float>(dy) / (scale * state->zoom);
}

static void printUsage(const char *argv0)
{
  std::cerr << "Usage: " << argv0 << " [options]\n"
            << "Options:\n"
            << "  -m, --moc3=FILE             Path to .moc3.json\n"
            << "  -r, --render-settings=FILE  Path to .moc3.render-settings.json\n"
            << "  -p, --parts=FILE            Path to .moc3.parts.json\n"
            << "  -t, --texture=FILE          Path to texture .png (override)\n"
            << "  -h, --help                  Show this help\n";
}

static bool parseOptionValue(const std::string &arg, const std::string &longName, std::string &out)
{
  const std::string prefix = "--" + longName + "=";
  if (arg.rfind(prefix, 0) == 0)
  {
    out = arg.substr(prefix.size());
    return true;
  }
  return false;
}

static bool parseShortOptionValue(const std::string &arg, const std::string &shortName, std::string &out)
{
  const std::string prefix = "-" + shortName + "=";
  if (arg.rfind(prefix, 0) == 0)
  {
    out = arg.substr(prefix.size());
    return true;
  }
  return false;
}

int main(int argc, char **argv)
{
  std::filesystem::path moc3JsonPath;
  std::filesystem::path renderSettingsPath;
  std::filesystem::path partsPath;
  std::filesystem::path textureOverridePath;

  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help")
    {
      printUsage(argv[0]);
      return 0;
    }

    std::string value;
    if (parseOptionValue(arg, "moc3", value) || parseShortOptionValue(arg, "m", value))
    {
      moc3JsonPath = value;
      continue;
    }
    if (parseOptionValue(arg, "render-settings", value) || parseShortOptionValue(arg, "r", value))
    {
      renderSettingsPath = value;
      continue;
    }
    if (parseOptionValue(arg, "parts", value) || parseShortOptionValue(arg, "p", value))
    {
      partsPath = value;
      continue;
    }
    if (parseOptionValue(arg, "texture", value) || parseShortOptionValue(arg, "t", value))
    {
      textureOverridePath = value;
      continue;
    }

    if ((arg == "-m" || arg == "--moc3") && i + 1 < argc)
    {
      moc3JsonPath = argv[++i];
      continue;
    }
    if ((arg == "-r" || arg == "--render-settings") && i + 1 < argc)
    {
      renderSettingsPath = argv[++i];
      continue;
    }
    if ((arg == "-p" || arg == "--parts") && i + 1 < argc)
    {
      partsPath = argv[++i];
      continue;
    }
    if ((arg == "-t" || arg == "--texture") && i + 1 < argc)
    {
      textureOverridePath = argv[++i];
      continue;
    }

    std::cerr << "Unknown option: " << arg << "\n";
    printUsage(argv[0]);
    return 1;
  }

  if (moc3JsonPath.empty())
  {
    // Default sample relative to build/ directory
    moc3JsonPath = std::filesystem::path("../live2d-assets/mao_pro/mao_pro.moc3.json");
  }

  std::cerr << "Model JSON: " << moc3JsonPath << "\n";

  glfwSetErrorCallback(glfwError); // set callback first

  if (!glfwInit())
  { // <-- missing call
    std::cerr << "Failed to init GLFW\n";
    return -1;
  }

  glfwDefaultWindowHints();

  // First try 4.1 Core
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);
  glfwWindowHint(GLFW_SAMPLES, 0);

  GLFWwindow *win = glfwCreateWindow(960, 540, "VTuber Base Engine", nullptr, nullptr);

  // Fallback to 3.2 Core if 4.1 fails
  if (!win)
  {
    std::cerr << "Retrying with 3.2 Core...\n";
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_SAMPLES, 0);
    win = glfwCreateWindow(960, 540, "VTuber Base Engine", nullptr, nullptr);
  }

  if (!win)
  {
    std::cerr << "Failed to create window\n";
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cerr << "Failed to init GLAD\n";
    return -1;
  }
#ifndef __APPLE__
  initDebug(); // skip on macOS to avoid INVALID_ENUM
#endif

  // after gladLoadGLLoader(...)
  std::cerr << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";
  std::cerr << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n";
  std::cerr << "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n";
  std::cerr << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
  std::cerr << "GLAD_GL_VERSION_3_0: " << GLAD_GL_VERSION_3_0 << "\n";
  if (!GLAD_GL_VERSION_3_0)
  {
    std::cerr << "No VAO support in this context.\n";
    return -1;
  }

  int stencilBits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
  GLenum err = glGetError();
  if (err == GL_INVALID_ENUM)
  {
    // No stencil in the default framebuffer; treat as 0
    stencilBits = 0;
    while (glGetError() != GL_NO_ERROR)
    {
    } // clear
  }
  std::cerr << "Stencil bits: " << stencilBits << "\n";

  Engine eng;
  std::unordered_map<std::string, std::filesystem::path> drawableTextures;
  bool modelLoaded = loadModelFromMoc3Json(moc3JsonPath, eng, drawableTextures, renderSettingsPath, partsPath);
  if (!eng.initGL())
    return -1;
  checkErr("after initGL");

  if (!textureOverridePath.empty())
  {
    if (!std::filesystem::exists(textureOverridePath))
    {
      std::cerr << "Texture override not found: " << textureOverridePath << "\n";
    }
    else
    {
      Texture tex = Texture().fromFilePath(textureOverridePath.string());
      if (tex.id)
      {
        eng.textures["tex_override"] = tex;
        for (auto &kv : eng.model.meshes)
          kv.second.texture_id = "tex_override";
        std::cerr << "Using texture override: " << textureOverridePath << "\n";
      }
      else
      {
        std::cerr << "Failed to load texture override: " << textureOverridePath << "\n";
      }
    }
  }

  size_t loadedTextureCount = 0;
  for (const auto &kv : drawableTextures)
  {
    if (kv.second.empty() || !std::filesystem::exists(kv.second))
      continue;
    Texture tex = Texture().fromFilePath(kv.second.string());
    if (!tex.id)
      continue;
    eng.textures[kv.first] = tex;
    ++loadedTextureCount;
    std::cerr << "Loaded texture " << kv.second << " as " << kv.first << "\n";
  }
  bool texLoaded = loadedTextureCount > 0;
  if (!texLoaded)
  {
    std::vector<std::filesystem::path> atlasJsonCandidates;
    atlasJsonCandidates.push_back(moc3JsonPath);
    atlasJsonCandidates.push_back(std::filesystem::path("../moc3-parser/out.json"));
    atlasJsonCandidates.push_back(std::filesystem::path("../../moc3-parser/out.json"));
    atlasJsonCandidates.push_back(std::filesystem::path("out.json"));

    for (const auto &p : atlasJsonCandidates)
    {
      if (!p.empty() && std::filesystem::exists(p))
      {
        texLoaded = loadAtlasTextureFromJson(eng, p.string(), "tex_checker");
        if (texLoaded)
          break;
      }
    }
  }

  bool needsChecker = false;
  for (auto &kv : eng.model.meshes)
  {
    if (eng.textures.find(kv.second.texture_id) == eng.textures.end())
    {
      needsChecker = true;
      kv.second.texture_id = "tex_checker";
    }
  }
  if (needsChecker && eng.textures.find("tex_checker") == eng.textures.end())
  {
    eng.createCheckerTexture("tex_checker", 64, 64);
    std::cerr << "Falling back to procedural checker texture.\n";
  }
  checkErr("after createCheckerTexture");

  if (!modelLoaded)
  {
    std::cerr << "Falling back to sample quad model.\n";
    makeSampleModel(eng.model);
  }

  eng.buildGLMeshes();
  checkErr("after buildGLMeshes");

  // init spring
  eng.springs["ParamMouthOpen"].reset(eng.model.params["ParamMouthOpen"].cur_v);

  ViewerState viewState;
  viewState.engine = &eng;
  glfwSetWindowUserPointer(win, &viewState);
  glfwSetScrollCallback(win, scrollCallback);
  glfwSetMouseButtonCallback(win, mouseButtonCallback);
  glfwSetCursorPosCallback(win, cursorPosCallback);

  double last = glfwGetTime();
  double start = last;
  while (!glfwWindowShouldClose(win))
  {
    glfwPollEvents();
    double now = glfwGetTime();
    float dt = float(now - last);
    last = now;

    int fbw, fbh;
    glfwGetFramebufferSize(win, &fbw, &fbh);
    viewState.fbw = std::max(1, fbw);
    viewState.fbh = std::max(1, fbh);

    eng.view = glm::translate(glm::mat4(1.0f), glm::vec3(viewState.pan, 0.0f));
    eng.view = glm::scale(eng.view, glm::vec3(viewState.zoom, viewState.zoom, 1.0f));

    eng.update(float(now - start), dt);
    checkErr("update");
    eng.render(fbw, fbh);

    glfwSwapBuffers(win);
    checkErr("frame");
  }
  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}
