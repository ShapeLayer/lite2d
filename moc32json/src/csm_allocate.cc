#include "csm_allocate.h"

#include <cstdlib>
#include <iostream>

#include "debug.h"

// CSMAllocator

void* CSMAllocator::Allocate(const csm::csmSizeType size) {
  return std::malloc(size);
}
void CSMAllocator::Deallocate(void* memory) {
  std::free(memory);
}

void* CSMAllocator::AllocateAligned(const csm::csmSizeType size, const csm::csmUint32 alignment) {
  void* ptr = nullptr;
  if (posix_memalign(&ptr, alignment, size) != 0) {
    return nullptr;
  }
  return ptr;
}

void CSMAllocator::DeallocateAligned(void* alignedMemory) {
  std::free(alignedMemory);
}

// csm_allocator_controller

csm_allocator_controller::csm_allocator_controller()
  : moc()
  , model()
  , model_deleter() {
  opt.LogFunction = log_dbg;
  opt.LoggingLevel = csm::CubismFramework::Option::LogLevel_Info;
  model_deleter = [this](csm::CubismModel* modelPtr) {
    if (modelPtr && moc) {
      moc->DeleteModel(modelPtr);
    }
  };
}

/**
 * @return 0 on success, 1 on failure
 */
int csm_allocator_controller::startup() {
  if (!csm::CubismFramework::StartUp(&allocator, &opt)) {
    std::cerr << "CubismFramework::StartUp failed" << std::endl;
    return 1;
  }
  csm::CubismFramework::Initialize();
  return 0;
}

void csm_allocator_controller::cleanup() {
  moc.reset();
  model.reset();
  csm::CubismFramework::Dispose();
  csm::CubismFramework::CleanUp();
}

/**
 * @return 0 on success, 1 on failure
 */
int csm_allocator_controller::create_moc(const csm::csmByte* moc_bytes, csm::csmSizeInt size) {
  moc = std::unique_ptr<csm::CubismMoc, std::function<void(csm::CubismMoc*)>>(
    csm::CubismMoc::Create(moc_bytes, size),
    [](csm::CubismMoc* p) { if (p) csm::CubismMoc::Delete(p); }
  );
  if (!moc) {
    std::cerr << "CubismMoc::Create failed" << std::endl;
    cleanup();
    return 1;
  }

  model = std::unique_ptr<csm::CubismModel, std::function<void(csm::CubismModel*)>>(
    moc->CreateModel(),
    model_deleter
  );

  if (!model) {
    std::cerr << "CubismMoc::CreateModel failed" << std::endl;
    cleanup();
    return 1;
  }
  return 0;
}

void csm_allocator_controller::delete_model(csm::CubismModel* model_ptr) {
  if (moc && model_ptr) {
    moc->DeleteModel(model_ptr);
  }
}
