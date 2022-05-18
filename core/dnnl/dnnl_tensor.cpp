// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "dnnl_tensor.h"

namespace oidn {

  DNNLTensor::DNNLTensor(const Ref<DNNLDevice>& device, const TensorDesc& desc, Storage storage)
    : Tensor(device->newBuffer(desc.getByteSize(), storage), desc)
  {
    init(device, buffer->getData());
  }

  DNNLTensor::DNNLTensor(const Ref<DNNLDevice>& device, const TensorDesc& desc, void* data)
    : Tensor(device, desc)
  {
    init(device, data);
  }

  DNNLTensor::DNNLTensor(const Ref<DNNLDevice>& device, const dnnl::memory::desc& desc)
    : Tensor(device, TensorDesc({int64_t(desc.get_size())}, TensorLayout::x, DataType::UInt8)),
      mem(desc, device->getDNNLEngine()) {}

  DNNLTensor::DNNLTensor(const Ref<Buffer>& buffer, const TensorDesc& desc, size_t byteOffset)
    : Tensor(buffer, desc, byteOffset)
  {
    if (byteOffset + getByteSize() > buffer->getByteSize())
      throw Exception(Error::InvalidArgument, "buffer region out of range");

    init(dynamicRefCast<DNNLDevice>(device), buffer->getData() + byteOffset);
  }

  void DNNLTensor::init(const Ref<DNNLDevice>& device, void* data)
  {
    mem = dnnl::memory(toDNNL(getDesc()), device->getDNNLEngine(), data);
  }

  void DNNLTensor::updatePtr()
  {
    if (buffer)
    {
      if (byteOffset + getByteSize() > buffer->getByteSize())
        throw std::range_error("buffer region out of range");

      mem.set_data_handle(buffer->getData() + byteOffset);
    }
  }

} // namespace oidn