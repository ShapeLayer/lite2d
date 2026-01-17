#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

class Engine;

// Loads a model from a .moc3.json file into the Engine. Also returns a map of texture IDs to file paths.
bool loadModelFromMoc3Json(const std::filesystem::path &jsonPath,
                           Engine &eng,
                           std::unordered_map<std::string, std::filesystem::path> &drawableTextures,
                           const std::filesystem::path &renderSettingsPath = {},
                           const std::filesystem::path &partsPath = {});

// Loads an atlas texture from a Live2D atlas JSON file and registers it in the Engine.
bool loadAtlasTextureFromJson(Engine &eng,
                              const std::string &jsonFile,
                              const std::string &texId);
