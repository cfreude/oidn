// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../device.h"

#if !defined(OIDN_COMPILE_HIP)
  typedef struct ihipStream_t* hipStream_t;
#endif
typedef struct miopenHandle* miopenHandle_t;

namespace oidn {

#if defined(OIDN_COMPILE_HIP)
  // Main kernel functions
  namespace
  {
    template<typename F>
    __global__ void basicHIPKernel(WorkDim<1> globalSize, const F f)
    {
      WorkItem<1> it(globalSize);
      if (it.getId() < it.getRange())
        f(it);
    }

    template<typename F>
    __global__ void basicHIPKernel(WorkDim<2> globalSize, const F f)
    {
      WorkItem<2> it(globalSize);
      if (it.getId<0>() < it.getRange<0>() &&
          it.getId<1>() < it.getRange<1>())
        f(it);
    }

    template<typename F>
    __global__ void basicHIPKernel(WorkDim<3> globalSize, const F f)
    {
      WorkItem<3> it(globalSize);
      if (it.getId<0>() < it.getRange<0>() &&
          it.getId<1>() < it.getRange<1>() &&
          it.getId<2>() < it.getRange<2>())
        f(it);
    }

    template<int N, typename F>
    __global__ void groupHIPKernel(const F f)
    {
      f(WorkGroupItem<N>());
    }
  }

  void checkError(hipError_t error);
#endif

  class HIPDevice final : public Device
  { 
  public:
    static bool isSupported();

    explicit HIPDevice(hipStream_t stream = nullptr);
    ~HIPDevice();

    OIDN_INLINE hipStream_t getHIPStream() const { return stream; }
    OIDN_INLINE miopenHandle_t getMIOpenHandle() const { return miopenHandle; }

    void wait() override;

    // Ops
    std::shared_ptr<Conv> newConv(const ConvDesc& desc) override;
    std::shared_ptr<Pool> newPool(const PoolDesc& desc) override;
    std::shared_ptr<Upsample> newUpsample(const UpsampleDesc& desc) override;
    std::shared_ptr<Autoexposure> newAutoexposure(const ImageDesc& srcDesc) override;
    std::shared_ptr<InputProcess> newInputProcess(const InputProcessDesc& desc) override;
    std::shared_ptr<OutputProcess> newOutputProcess(const OutputProcessDesc& desc) override;
    std::shared_ptr<ImageCopy> newImageCopy() override;

    // Memory
    void* malloc(size_t byteSize, Storage storage) override;
    void free(void* ptr, Storage storage) override;
    void memcpy(void* dstPtr, const void* srcPtr, size_t byteSize) override;
    Storage getPointerStorage(const void* ptr) override;

  #if defined(OIDN_COMPILE_HIP)
    // Enqueues a basic kernel
    template<int N, typename F>
    OIDN_INLINE void runKernelAsync(WorkDim<N> globalSize, const F& f)
    {
      // TODO: improve group size computation
      WorkDim<N> groupSize = suggestWorkGroupSize(globalSize);
      WorkDim<N> numGroups = ceil_div(globalSize, groupSize);

      basicHIPKernel<<<numGroups, groupSize, 0, stream>>>(globalSize, f);
      checkError(hipGetLastError());
    }

    // Enqueues a work-group kernel
    template<int N, typename F>
    OIDN_INLINE void runKernelAsync(WorkDim<N> numGroups, WorkDim<N> groupSize, const F& f)
    {
      groupHIPKernel<N><<<numGroups, groupSize, 0, stream>>>(f);
      checkError(hipGetLastError());
    }
  #endif

    // Enqueues a host function
    void runHostFuncAsync(std::function<void()>&& f) override;

  private:
    void init() override;

    WorkDim<1> suggestWorkGroupSize(WorkDim<1> globalSize) { return 1024; }
    WorkDim<2> suggestWorkGroupSize(WorkDim<2> globalSize) { return {32, 32}; }
    WorkDim<3> suggestWorkGroupSize(WorkDim<3> globalSize) { return {1, 32, 32}; }

    int deviceId = -1;
    hipStream_t stream = nullptr;
    miopenHandle_t miopenHandle = nullptr;
  };

} // namespace oidn