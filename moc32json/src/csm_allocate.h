#ifndef __MOC32JSON_CSM_ALLOCATE_H__
#define __MOC32JSON_CSM_ALLOCATE_H__

#include <memory>
#include <functional>

#include "CubismFramework.hpp"
#include "Model/CubismMoc.hpp"

namespace csm = Live2D::Cubism::Framework;

class CSMAllocator : public csm::ICubismAllocator {
public:
  void* Allocate(const csm::csmSizeType size) override;
  void Deallocate(void* memory) override;
  void* AllocateAligned(const csm::csmSizeType size, const csm::csmUint32 alignment) override;
  void DeallocateAligned(void* alignedMemory) override;
};

class csm_allocator_controller {
public:
  CSMAllocator allocator;
  csm::CubismFramework::Option opt{};
  std::unique_ptr<csm::CubismMoc, std::function<void(csm::CubismMoc*)>> moc;
  std::function<void(csm::CubismModel*)> model_deleter;
  std::unique_ptr<csm::CubismModel, std::function<void(csm::CubismModel*)>> model;

  csm_allocator_controller();
  int startup();
  void cleanup();
  int create_moc(const csm::csmByte* moc_bytes, csm::csmSizeInt size);
  void delete_model(csm::CubismModel* model_ptr);
};

#endif  // __MOC32JSON_CSM_ALLOCATE_H__
