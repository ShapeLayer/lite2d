#include "csm_serialize.h"

#include "CubismCdiJson.hpp"
#include "CubismModelSettingJson.hpp"
#include "Id/CubismId.hpp"
#include "Id/CubismIdManager.hpp"
#include "Model/CubismModel.hpp"

csm_serialize_controller::csm_serialize_controller() {
  output["canvas"] = {
    { "width", -1 },
    { "height", -1 }
  };
}

/**
 * @return 0 on success, 1 on failure
 */
int csm_serialize_controller::fetch_value(
  std::unique_ptr<csm::CubismModel, std::function<void(csm::CubismModel*)>> model
) {
  output["canvas"] = {
    { "width", model->GetCanvasWidth() },
    { "height", model->GetCanvasHeight() }
  };
  const csm::csmInt32 part_count = model->GetPartCount();
  json parts = json::array();
  const csm::csmInt32* part_parent_indices = model->GetPartParentPartIndices();
  for (csm::csmInt32 i = 0; i < part_count; ++i) {
    const csm::CubismIdHandle part_id = model->GetPartId(static_cast<csm::csmUint32>(i));
    const std::string part_id_str = part_id ? part_id->GetString().GetRawString() : "";
    const csm::csmInt32 parent_part_index = part_parent_indices ? part_parent_indices[i] : -1;

    parts.push_back({
      { "index", i },
      { "id", part_id_str },
      { "parent_part_index", parent_part_index },
      { "opacity", model->GetPartOpacity(i) },
    });
  }
  output["parts"] = parts;

  const csm::csmInt32 parameter_count = model->GetParameterCount();
  json parameters = json::array();
  for (csm::csmInt32 i = 0; i < parameter_count; ++i) {
    const csm::CubismIdHandle parameter_id = model->GetParameterId(static_cast<csm::csmUint32>(i));
    const std::string parameter_id_str = parameter_id ? parameter_id->GetString().GetRawString() : "";

    parameters.push_back({
      { "index", i },
      { "id", parameter_id_str },
      { "minimum", model->GetParameterMinimumValue(static_cast<csm::csmUint32>(i)) },
      { "maximum", model->GetParameterMaximumValue(static_cast<csm::csmUint32>(i)) },
      { "default", model->GetParameterDefaultValue(static_cast<csm::csmUint32>(i)) },
      { "type", static_cast<int>(model->GetParameterType(static_cast<csm::csmUint32>(i))) },
    });
  }
  output["parameters"] = parameters;

  drawable_count = model->GetDrawableCount();
  drawables = json::array();
  const csm::csmInt32* draw_orders = Live2D::Cubism::Core::csmGetDrawableDrawOrders(model->GetModel());
  const csm::csmInt32* render_orders = model->GetDrawableRenderOrders();
  const csm::csmInt32* mask_counts = model->GetDrawableMaskCounts();
  const csm::csmInt32** masks = model->GetDrawableMasks();
  
  for (csm::csmInt32 i = 0; i < drawable_count; ++i) {
    const csm::CubismIdHandle id_handle = model->GetDrawableId(i);
    const std::string drawable_id = id_handle ? id_handle->GetString().GetRawString() : "";

    const csm::csmInt32 vertex_count = model->GetDrawableVertexCount(i);
    const csm::csmInt32 index_count = model->GetDrawableVertexIndexCount(i);

    json positions = json::array();
    json uvs = json::array();
    const Live2D::Cubism::Core::csmVector2* position_array = model->GetDrawableVertexPositions(i);
    const Live2D::Cubism::Core::csmVector2* uv_array = model->GetDrawableVertexUvs(i);

    for (csm::csmInt32 v = 0; v < vertex_count; ++v) {
      positions.push_back({ position_array[v].X, position_array[v].Y });
      uvs.push_back({ uv_array[v].X, uv_array[v].Y });
    }

    json indices = json::array();
    const csm::csmUint16 *index_array = model->GetDrawableVertexIndices(i);
    for (csm::csmInt32 idx = 0; idx < index_count; ++idx) {
      indices.push_back(index_array[idx]);
    }

    json drawable_masks = json::array();
    const csm::csmInt32 mask_count = mask_counts ? mask_counts[i] : 0;
    if (mask_count > 0 && masks && masks[i]) {
      for (csm::csmInt32 m = 0; m < mask_count; ++m) {
        drawable_masks.push_back(masks[i][m]);
      }
    }

    drawables.push_back({
      { "id", drawable_id },
      { "texture_index", model->GetDrawableTextureIndex(i) },
      { "parent_part_index", model->GetDrawableParentPartIndex(static_cast<csm::csmUint32>(i)) },
      { "draw_order", draw_orders ? draw_orders[i] : 0 },
      { "render_order", render_orders ? render_orders[i] : 0 },
      { "vertex_count", vertex_count },
      { "index_count", index_count },
      { "positions", positions },
      { "uvs", uvs },
      { "indices", indices },
      { "mask_count", mask_count },
      { "masks", drawable_masks },
      { "inverted_mask", model->GetDrawableInvertedMask(i) },
      { "opacity", model->GetDrawableOpacity(i) },
      { "blend_mode", static_cast<int>(model->GetDrawableBlendMode(i)) },
    });
  }
  output["drawables"] = drawables;
  return 0;
}

int csm_serialize_controller::add_model3(const std::vector<csm::csmByte>& model3_bytes)
{
  if (model3_bytes.empty()) {
    return 1;
  }

  csm::CubismModelSettingJson model_setting(model3_bytes.data(), static_cast<csm::csmSizeInt>(model3_bytes.size()));

  json model3 = json::object();
  json eye_blink_ids = json::array();
  const csm::csmInt32 eye_blink_count = model_setting.GetEyeBlinkParameterCount();
  for (csm::csmInt32 i = 0; i < eye_blink_count; ++i) {
    const csm::CubismIdHandle id_handle = model_setting.GetEyeBlinkParameterId(i);
    const std::string id = id_handle ? id_handle->GetString().GetRawString() : "";
    eye_blink_ids.push_back(id);
  }

  json lip_sync_ids = json::array();
  const csm::csmInt32 lip_sync_count = model_setting.GetLipSyncParameterCount();
  for (csm::csmInt32 i = 0; i < lip_sync_count; ++i) {
    const csm::CubismIdHandle id_handle = model_setting.GetLipSyncParameterId(i);
    const std::string id = id_handle ? id_handle->GetString().GetRawString() : "";
    lip_sync_ids.push_back(id);
  }

  model3["eye_blink_ids"] = eye_blink_ids;
  model3["lip_sync_ids"] = lip_sync_ids;
  output["model3"] = model3;

  return 0;
}

int csm_serialize_controller::add_cdi3(const std::vector<csm::csmByte>& cdi3_bytes)
{
  if (cdi3_bytes.empty()) {
    return 1;
  }

  csm::CubismCdiJson cdi(cdi3_bytes.data(), static_cast<csm::csmSizeInt>(cdi3_bytes.size()));

  json display_info = json::object();

  json parameters = json::array();
  const csm::csmInt32 parameter_count = cdi.GetParametersCount();
  for (csm::csmInt32 i = 0; i < parameter_count; ++i) {
    parameters.push_back({
      { "id", cdi.GetParametersId(i) },
      { "group_id", cdi.GetParametersGroupId(i) },
      { "name", cdi.GetParametersName(i) },
    });
  }

  json parameter_groups = json::array();
  const csm::csmInt32 parameter_group_count = cdi.GetParameterGroupsCount();
  for (csm::csmInt32 i = 0; i < parameter_group_count; ++i) {
    parameter_groups.push_back({
      { "id", cdi.GetParameterGroupsId(i) },
      { "group_id", cdi.GetParameterGroupsGroupId(i) },
      { "name", cdi.GetParameterGroupsName(i) },
    });
  }

  json parts = json::array();
  const csm::csmInt32 parts_count = cdi.GetPartsCount();
  for (csm::csmInt32 i = 0; i < parts_count; ++i) {
    parts.push_back({
      { "id", cdi.GetPartsId(i) },
      { "name", cdi.GetPartsName(i) },
    });
  }

  display_info["parameters"] = parameters;
  display_info["parameter_groups"] = parameter_groups;
  display_info["parts"] = parts;

  output["display_info"] = display_info;
  return 0;
}
