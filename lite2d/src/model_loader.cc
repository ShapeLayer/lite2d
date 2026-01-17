#include "model_loader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include <glm/glm.hpp>
#include "commons/json.hpp"

#include "deformer.h"
#include "engine.h"
#include "model.h"
#include "texture.h"

using json = nlohmann::json;

namespace
{
struct RenderSettings
{
  std::vector<std::string> order;
  std::unordered_set<std::string> hidden;
};

struct FacePartsSettings
{
  std::unordered_map<std::string, std::vector<std::string>> parts;
};

struct BodyPartsSettings
{
  std::unordered_map<std::string, std::vector<std::string>> parts;
  std::unordered_map<std::string, std::vector<std::string>> seams;
};

bool endsWith(const std::string &value, const std::string &suffix)
{
  if (suffix.size() > value.size())
    return false;
  return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

std::filesystem::path getRenderSettingsPath(const std::filesystem::path &moc3JsonPath)
{
  const std::string suffix = ".moc3.json";
  const std::string pathStr = moc3JsonPath.string();
  if (endsWith(pathStr, suffix))
    return std::filesystem::path(pathStr.substr(0, pathStr.size() - suffix.size()) + ".moc3.render-settings.json");
  return std::filesystem::path(pathStr + ".moc3.render-settings.json");
}

std::filesystem::path getPartsPath(const std::filesystem::path &moc3JsonPath)
{
  const std::string suffix = ".moc3.json";
  const std::string pathStr = moc3JsonPath.string();
  if (endsWith(pathStr, suffix))
    return std::filesystem::path(pathStr.substr(0, pathStr.size() - suffix.size()) + ".moc3.parts.json");
  return std::filesystem::path(pathStr + ".moc3.parts.json");
}

RenderSettings loadRenderSettings(const std::filesystem::path &moc3JsonPath,
                                  const std::filesystem::path &explicitPath)
{
  RenderSettings settings;
  const auto settingsPath = explicitPath.empty() ? getRenderSettingsPath(moc3JsonPath)
                                                 : explicitPath;
  std::ifstream ifs(settingsPath);
  if (!ifs)
    return settings;
  try
  {
    nlohmann::json j;
    ifs >> j;
    if (j.contains("order") && j["order"].is_array())
    {
      for (const auto &id : j["order"])
      {
        if (id.is_string())
          settings.order.push_back(id.get<std::string>());
      }
    }
    if (j.contains("hidden") && j["hidden"].is_array())
    {
      for (const auto &id : j["hidden"])
      {
        if (id.is_string())
          settings.hidden.insert(id.get<std::string>());
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to parse render settings: " << e.what() << "\n";
  }
  return settings;
}

FacePartsSettings loadFacePartsSettings(const std::filesystem::path &moc3JsonPath,
                                        const std::filesystem::path &explicitPath)
{
  FacePartsSettings settings;
  const auto settingsPath = explicitPath.empty() ? getPartsPath(moc3JsonPath)
                                                 : explicitPath;
  std::ifstream ifs(settingsPath);
  if (!ifs)
    return settings;
  try
  {
    nlohmann::json j;
    ifs >> j;
    if (j.contains("face_parts") && j["face_parts"].is_object())
    {
      const auto &partsObj = j["face_parts"];
      for (auto it = partsObj.begin(); it != partsObj.end(); ++it)
      {
        if (!it.value().is_array())
          continue;
        std::vector<std::string> ids;
        for (const auto &id : it.value())
        {
          if (id.is_string())
            ids.push_back(id.get<std::string>());
        }
        if (!ids.empty())
          settings.parts[it.key()] = std::move(ids);
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to parse face parts settings: " << e.what() << "\n";
  }
  return settings;
}

BodyPartsSettings loadBodyPartsSettings(const std::filesystem::path &moc3JsonPath,
                                        const std::filesystem::path &explicitPath)
{
  BodyPartsSettings settings;
  std::filesystem::path settingsPath = explicitPath;
  if (settingsPath.empty())
  {
    settingsPath = getPartsPath(moc3JsonPath);
  }

  std::ifstream ifs(settingsPath);
  if (!ifs)
    return settings;
  try
  {
    nlohmann::json j;
    ifs >> j;
    if (j.contains("body_parts") && j["body_parts"].is_object())
    {
      const auto &partsObj = j["body_parts"];
      for (auto it = partsObj.begin(); it != partsObj.end(); ++it)
      {
        if (!it.value().is_array())
          continue;
        std::vector<std::string> ids;
        for (const auto &id : it.value())
        {
          if (id.is_string())
            ids.push_back(id.get<std::string>());
        }
        if (!ids.empty())
          settings.parts[it.key()] = std::move(ids);
      }
    }
    if (j.contains("seam_parts") && j["seam_parts"].is_object())
    {
      const auto &seamObj = j["seam_parts"];
      for (auto it = seamObj.begin(); it != seamObj.end(); ++it)
      {
        if (!it.value().is_array())
          continue;
        std::vector<std::string> ids;
        for (const auto &id : it.value())
        {
          if (id.is_string())
            ids.push_back(id.get<std::string>());
        }
        if (!ids.empty())
          settings.seams[it.key()] = std::move(ids);
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to parse body parts settings: " << e.what() << "\n";
  }
  return settings;
}

std::string readTexturePathFromJson(const std::string &jsonFile)
{
  std::ifstream ifs(jsonFile);
  if (!ifs)
  {
    std::cerr << "Cannot open atlas json: " << jsonFile << "\n";
    return "";
  }
  try
  {
    nlohmann::json j;
    ifs >> j;
    if (j.contains("texture") && j["texture"].is_string())
      return j["texture"].get<std::string>();
    std::cerr << "No \"texture\" string field in " << jsonFile << "\n";
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to parse json " << jsonFile << ": " << e.what() << "\n";
  }
  return "";
}
} // namespace

bool loadAtlasTextureFromJson(Engine &eng,
                              const std::string &jsonFile,
                              const std::string &texId)
{
  std::string texRel = readTexturePathFromJson(jsonFile);
  if (texRel.empty())
    return false;

  auto tryLoad = [&](const std::filesystem::path &p) -> bool
  {
    if (p.empty())
      return false;
    Texture t = Texture().fromFilePath(p.string());
    if (!t.id)
      return false;
    eng.textures[texId] = t;
    std::cerr << "Loaded atlas texture from " << p << "\n";
    return true;
  };

  std::filesystem::path baseDir = std::filesystem::path(jsonFile).parent_path();
  std::filesystem::path texPath = baseDir / texRel;

  if (std::filesystem::exists(texPath) && tryLoad(texPath))
    return true;

  // Fallback: look for texture_00.png near the moc3 json directory
  std::filesystem::path fallback;
  std::string targetName = "texture_00.png";
  if (std::filesystem::is_directory(baseDir))
  {
    for (auto it = std::filesystem::recursive_directory_iterator(baseDir);
         it != std::filesystem::recursive_directory_iterator(); ++it)
    {
      if (!it->is_regular_file())
        continue;
      if (it->path().filename() == targetName)
      {
        fallback = it->path();
        break;
      }
    }
  }

  if (!fallback.empty() && tryLoad(fallback))
    return true;

  std::cerr << "Failed to load texture at " << texPath << " and no fallback found\n";
  return false;
}

bool loadModelFromMoc3Json(const std::filesystem::path &jsonPath,
                           Engine &eng,
                           std::unordered_map<std::string, std::filesystem::path> &drawableTextures,
                           const std::filesystem::path &renderSettingsPath,
                           const std::filesystem::path &partsPath)
{
  std::ifstream ifs(jsonPath);
  if (!ifs)
  {
    std::cerr << "Cannot open model json: " << jsonPath << "\n";
    return false;
  }

  json j;
  try
  {
    ifs >> j;
  }
  catch (const std::exception &e)
  {
    std::cerr << "JSON parse error in " << jsonPath << ": " << e.what() << "\n";
    return false;
  }

  if (!j.contains("drawables") || !j["drawables"].is_array())
  {
    std::cerr << "No drawables array in " << jsonPath << "\n";
    return false;
  }

  drawableTextures.clear();
  const RenderSettings renderSettings = loadRenderSettings(jsonPath, renderSettingsPath);
  const FacePartsSettings facePartsSettings = loadFacePartsSettings(jsonPath, partsPath);
  const BodyPartsSettings bodyPartsSettings = loadBodyPartsSettings(jsonPath, partsPath);
  std::unordered_map<std::string, int> orderIndex;
  orderIndex.reserve(renderSettings.order.size());
  for (size_t i = 0; i < renderSettings.order.size(); ++i)
  {
    int reversedIndex = static_cast<int>(renderSettings.order.size() - 1 - i);
    orderIndex[renderSettings.order[i]] = reversedIndex;
  }
  int fallbackOrder = renderSettings.order.empty() ? 0 : -1;
  std::filesystem::path baseDir = jsonPath.parent_path();
  std::unordered_map<int, std::filesystem::path> indexedTextures;
  if (!baseDir.empty())
  {
    for (auto it = std::filesystem::recursive_directory_iterator(baseDir);
         it != std::filesystem::recursive_directory_iterator(); ++it)
    {
      if (!it->is_regular_file())
        continue;
      std::string name = it->path().filename().string();
      if (name.rfind("texture_", 0) != 0)
        continue;
      size_t dot = name.find_last_of('.');
      if (dot == std::string::npos || dot <= 8)
        continue;
      std::string indexStr = name.substr(8, dot - 8);
      try
      {
        int index = std::stoi(indexStr);
        indexedTextures[index] = it->path();
      }
      catch (...)
      {
      }
    }
  }

  float canvas_w = j.value("canvas", json::object()).value("width", 2.0f);
  float canvas_h = j.value("canvas", json::object()).value("height", 2.0f);

  // First pass: compute global bounding box to center the model.
  glm::vec2 bbMin(1e9f), bbMax(-1e9f);
  size_t totalVerts = 0;
  for (const auto &drawable : j["drawables"])
  {
    if (!drawable.contains("positions") || !drawable["positions"].is_array())
      continue;
    for (const auto &p : drawable["positions"])
    {
      if (!p.is_array() || p.size() < 2)
        continue;
      float px = p[0].get<float>();
      float py = p[1].get<float>();
      bbMin.x = std::min(bbMin.x, px);
      bbMin.y = std::min(bbMin.y, py);
      bbMax.x = std::max(bbMax.x, px);
      bbMax.y = std::max(bbMax.y, py);
      ++totalVerts;
    }
  }

  if (totalVerts == 0)
  {
    std::cerr << "No vertices found in " << jsonPath << "\n";
    return false;
  }

  glm::vec2 bbCenter = (bbMin + bbMax) * 0.5f;
  glm::vec2 bbSize = bbMax - bbMin;

  eng.model.meshes.clear();
  eng.model.deformers.clear();
  eng.model.mesh_face_parts.clear();
  eng.model.mesh_body_parts.clear();
  eng.model.mesh_seam_parts.clear();

  Deformer root;
  root.id = "def_root";
  eng.model.deformers.emplace(root.id, root);

  int meshCounter = 0;
  for (const auto &drawable : j["drawables"])
  {
    if (!drawable.contains("positions") || !drawable.contains("indices"))
      continue;

    const auto &posArr = drawable["positions"];
    const auto &idxArr = drawable["indices"];
    const auto &uvArr = drawable.contains("uvs") ? drawable["uvs"] : json::array();
    if (!posArr.is_array() || !idxArr.is_array())
      continue;

    size_t vcount = posArr.size();
    if (vcount == 0)
      continue;

    auto normPos = [&](float x, float y) -> glm::vec2
    {
      // Center around global bbox; keep model Y as-is to avoid vertical flip.
      float nx = (x - bbCenter.x);
      float ny = (y - bbCenter.y);
      return glm::vec2(nx, ny);
    };

    ArtMesh mesh;
    mesh.id = drawable.value("id", std::string("mesh_") + std::to_string(meshCounter));
    if (!renderSettings.hidden.empty() && renderSettings.hidden.count(mesh.id))
      mesh.visible = false;
    int tex_idx = drawable.value("texture_index", 0);
    std::string texId = std::string("tex_idx_") + std::to_string(tex_idx);
    mesh.texture_id = texId;
    auto itTex = indexedTextures.find(tex_idx);
    if (itTex != indexedTextures.end())
      drawableTextures.try_emplace(texId, itTex->second);
    auto itOrder = orderIndex.find(mesh.id);
    mesh.draw_order = (itOrder != orderIndex.end()) ? itOrder->second : fallbackOrder++;
    mesh.blend_mode = drawable.value("blend_mode", 0);
    mesh.opacity = drawable.value("opacity", 1.0f);
    mesh.deformers = {root.id};

    mesh.verts.reserve(vcount);
    for (size_t i = 0; i < vcount; ++i)
    {
      const auto &p = posArr[i];
      if (!p.is_array() || p.size() < 2)
        continue;
      float px = p[0].get<float>();
      float py = p[1].get<float>();

      Vertex vtx;
      vtx.pos = normPos(px, py);

      float ux = 0.0f;
      float uy = 0.0f;
      if (i < uvArr.size() && uvArr[i].is_array() && uvArr[i].size() >= 2)
      {
        ux = uvArr[i][0].get<float>();
        uy = uvArr[i][1].get<float>();
      }
      vtx.uv = {ux, 1.0f - uy};
      vtx.color = {1.0f, 1.0f, 1.0f};
      vtx.bone = {0, 0};
      vtx.weight = {1.0f, 0.0f};
      mesh.verts.push_back(vtx);
    }

    mesh.indices.reserve(idxArr.size());
    for (const auto &idxVal : idxArr)
    {
      if (!idxVal.is_number())
        continue;
      int iv = idxVal.get<int>();
      if (iv >= 0 && iv < static_cast<int>(mesh.verts.size()))
        mesh.indices.push_back(static_cast<uint32_t>(iv));
    }

    if (mesh.verts.empty() || mesh.indices.size() < 3)
      continue;

    eng.model.meshes.emplace(mesh.id, mesh);
    eng.model.deformers[root.id].bound_meshes.push_back(mesh.id);
    ++meshCounter;
  }

  if (!facePartsSettings.parts.empty())
  {
    for (const auto &kv : facePartsSettings.parts)
    {
      for (const auto &meshId : kv.second)
      {
        eng.model.mesh_face_parts[meshId].insert(kv.first);
      }
    }
    std::unordered_set<std::string> tags;
    std::unordered_set<std::string> meshes;
    for (const auto &kv : eng.model.mesh_face_parts)
    {
      meshes.insert(kv.first);
      for (const auto &tag : kv.second)
        tags.insert(tag);
    }
    const bool hasFaceTags = tags.count("eye_left") || tags.count("eye_right") || tags.count("eye")
                             || tags.count("mouth") || tags.count("brow_left") || tags.count("brow_right")
                             || tags.count("eye_white_left") || tags.count("eye_white_right")
                             || tags.count("eye_ball_left") || tags.count("eye_ball_right");
    std::cerr << "Face parts mapping loaded: " << tags.size() << " tags, " << meshes.size() << " meshes";
    if (hasFaceTags)
      std::cerr << " (face elements detected)";
    std::cerr << "\n";
  }
  else
  {
    std::cerr << "No face parts mapping found (face elements not identified).\n";
  }

  if (!bodyPartsSettings.parts.empty())
  {
    for (const auto &kv : bodyPartsSettings.parts)
    {
      for (const auto &meshId : kv.second)
      {
        eng.model.mesh_body_parts[meshId].insert(kv.first);
      }
    }
    std::cerr << "Body parts mapping loaded: " << bodyPartsSettings.parts.size() << " tags\n";
  }

  if (!bodyPartsSettings.seams.empty())
  {
    for (const auto &kv : bodyPartsSettings.seams)
    {
      for (const auto &meshId : kv.second)
      {
        eng.model.mesh_seam_parts[meshId].insert(kv.first);
      }
    }
    std::cerr << "Seam parts mapping loaded: " << bodyPartsSettings.seams.size() << " tags\n";
  }

  if (bodyPartsSettings.parts.empty() && bodyPartsSettings.seams.empty() && !facePartsSettings.parts.empty())
  {
    const std::unordered_set<std::string> bodyTags = {
        "head", "neck", "shoulder_left", "shoulder_right", "torso", "chest", "body",
        "hair", "hair_front", "hair_back", "hair_side", "ear_left", "ear_right"};
    const std::unordered_set<std::string> seamTags = {"neck_seam", "jaw_seam"};
    for (const auto &kv : facePartsSettings.parts)
    {
      if (bodyTags.count(kv.first))
      {
        for (const auto &meshId : kv.second)
          eng.model.mesh_body_parts[meshId].insert(kv.first);
      }
      if (seamTags.count(kv.first))
      {
        for (const auto &meshId : kv.second)
          eng.model.mesh_seam_parts[meshId].insert(kv.first);
      }
    }
  }

  // Use whichever is larger: declared canvas or actual bbox size, to keep aspect-fit sane.
  eng.canvas = {std::max(canvas_w, bbSize.x), std::max(canvas_h, bbSize.y)};

  if (meshCounter == 0)
  {
    std::cerr << "No usable drawables found in " << jsonPath << "\n";
    return false;
  }

  std::cerr << "Loaded " << meshCounter << " drawables from " << jsonPath << "\n";
  return true;
}
