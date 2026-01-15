#include "csm_serialize.h"

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
  drawable_count = model->GetDrawableCount();
  drawables = json::array();
  
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

    drawables.push_back({
      { "id", drawable_id },
      { "texture_index", model->GetDrawableTextureIndex(i) },
      { "parent_part_index", model->GetDrawableParentPartIndex(static_cast<Csm::csmUint32>(i)) },
      { "vertex_count", vertex_count },
      { "index_count", index_count },
      { "positions", positions },
      { "uvs", uvs },
      { "indices", indices },
      { "opacity", model->GetDrawableOpacity(i) },
      { "blend_mode", static_cast<int>(model->GetDrawableBlendMode(i)) },
    });
  }
  output["drawables"] = drawables;
  return 0;
}
