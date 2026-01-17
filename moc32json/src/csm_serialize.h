#ifndef __MOC32JSON_CSM_SERIALIZE_H__
#define __MOC32JSON_CSM_SERIALIZE_H__

#include <vector>

#include "json.hpp"

#include "csm_allocate.h"

using json = nlohmann::json;

class csm_serialize_controller {
public:
  json output;
  json drawables;
  csm::csmInt32 drawable_count;

  csm_serialize_controller();
  int fetch_value(
    std::unique_ptr<csm::CubismModel, std::function<void(csm::CubismModel*)>> model
  );
  int add_model3(const std::vector<csm::csmByte>& model3_bytes);
  int add_cdi3(const std::vector<csm::csmByte>& cdi3_bytes);
};

#endif  // __MOC32JSON_CSM_SERIALIZE_H__
