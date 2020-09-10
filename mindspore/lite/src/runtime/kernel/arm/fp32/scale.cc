/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/runtime/kernel/arm/fp32/scale.h"
#include <string.h>
#include <vector>
#include "schema/model_generated.h"
#include "src/kernel_registry.h"
#include "include/errorcode.h"
#include "src/runtime/runtime_api.h"

using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Scale;

namespace mindspore::kernel {
ScaleCPUKernel::~ScaleCPUKernel() {
  if (scale_param_->const_scale_) {
    if (scale_ != nullptr) {
      free(scale_);
      scale_ = nullptr;
    }
  }
  if (offset_ != nullptr) {
    free(offset_);
    offset_ = nullptr;
  }
}

int ScaleCPUKernel::InitScaleOffset() {
  auto scale_tensor = in_tensors_.at(1);
  float *scale_ptr = reinterpret_cast<float *>(in_tensors_.at(1)->MutableData());
  if (scale_ptr != nullptr) {
    scale_param_->const_scale_ = true;
    scale_ = reinterpret_cast<float *>(malloc(scale_tensor->ElementsNum() * sizeof(float)));
    if (scale_ == nullptr) {
      MS_LOG(ERROR) << "Malloc buffer failed.";
      return RET_ERROR;
    }
    memcpy(scale_, scale_ptr, scale_tensor->ElementsNum() * sizeof(float));
  } else {
    scale_param_->const_scale_ = false;
    scale_ = nullptr;
  }

  offset_ = reinterpret_cast<float *>(malloc(scale_param_->axis_size_ * sizeof(float)));
  if (offset_ == nullptr) {
    MS_LOG(ERROR) << "Malloc buffer failed.";
    return RET_ERROR;
  }
  memset(offset_, 0, scale_param_->axis_size_ * sizeof(float));
  if (in_tensors_.size() == 3) {
    auto offset_tensor = in_tensors_.at(2);
    memcpy(offset_, offset_tensor->MutableData(), offset_tensor->ElementsNum() * sizeof(float));
  }
  return RET_OK;
}

int ScaleCPUKernel::InitParameter() {
  auto in_tensor = in_tensors_.at(0);
  auto in_shape = in_tensor->shape();
  auto scale_tensor = in_tensors_.at(1);
  auto scale_shape = scale_tensor->shape();

  if (scale_param_->axis_ < 0) {
    scale_param_->axis_ = scale_param_->axis_ + in_shape.size();
  }
  if (scale_shape.size() + scale_param_->axis_ > in_shape.size()) {
    MS_LOG(ERROR) << "Scale tensor shape is incorrect.";
    return RET_ERROR;
  }
  scale_param_->outer_size_ = 1;
  scale_param_->axis_size_ = 1;
  scale_param_->inner_size_ = 1;
  for (int i = 0; i < scale_param_->axis_; i++) {
    scale_param_->outer_size_ *= in_shape[i];
  }
  for (size_t i = 0; i < scale_shape.size(); i++) {
    if (in_shape[i + scale_param_->axis_] != scale_shape[i]) {
      MS_LOG(ERROR) << "Scale tensor shape is incorrect.";
      return RET_ERROR;
    }
    scale_param_->axis_size_ *= in_shape[i + scale_param_->axis_];
  }
  for (size_t i = scale_param_->axis_ + scale_shape.size(); i < in_shape.size(); i++) {
    scale_param_->inner_size_ *= in_shape[i];
  }
  scale_param_->op_parameter_.thread_num_ = MSMIN(scale_param_->op_parameter_.thread_num_, scale_param_->outer_size_);
  return RET_OK;
}

int ScaleCPUKernel::Init() {
  if (in_tensors_.size() < 2 || in_tensors_.size() > 3) {
    MS_LOG(ERROR) << "inputs to Scale operator should be 2 or 3, but " << in_tensors_.size() << " is given.";
    return RET_ERROR;
  }

  if (!InferShapeDone()) {
    return RET_OK;
  }

  ReSize();
  return RET_OK;
}

int ScaleCPUKernel::ReSize() {
  auto ret = InitParameter();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Scale fp32 InitParameter failed.";
    return RET_ERROR;
  }

  ret = InitScaleOffset();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Scale fp32 InitScaleOffset failed.";
    return RET_ERROR;
  }
  return RET_OK;
}

int ScaleCPUKernel::Scale(int task_id) {
  DoScale(input_ptr_, output_ptr_, scale_, offset_, task_id, scale_param_);
  return RET_OK;
}

int ScaleRun(void *cdata, int task_id) {
  auto scale = reinterpret_cast<ScaleCPUKernel *>(cdata);
  auto ret = scale->Scale(task_id);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "ScaleRun error task_id[" << task_id << "] error_code[" << ret << "]";
    return RET_ERROR;
  }
  return RET_OK;
}

int ScaleCPUKernel::Run() {
  auto ret = Prepare();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Prepare fail!ret: " << ret;
    return ret;
  }
  auto in_tensor = in_tensors_.front();
  input_ptr_ = reinterpret_cast<float *>(in_tensor->MutableData());
  if (scale_ == nullptr) {
    auto scale_tensor = in_tensors_[1];
    scale_ = reinterpret_cast<float *>(scale_tensor->MutableData());
  }
  auto out_tensor = out_tensors_.front();
  output_ptr_ = reinterpret_cast<float *>(out_tensor->MutableData());

  ret = ParallelLaunch(THREAD_POOL_DEFAULT, ScaleRun, this, op_parameter_->thread_num_);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Scale error error_code[" << ret << "]";
    return RET_ERROR;
  }
  return RET_OK;
}
int RestoreScaleWeight(lite::Tensor *input_tensor) {
  MS_ASSERT(input_tensor != nullptr);
  if (input_tensor->data_type() != kNumberTypeUInt8) {
    MS_LOG(ERROR) << "mat mul input type error" << input_tensor->data_type();
    return RET_ERROR;
  }
  if (input_tensor->GetQuantParams().empty()) {
    MS_LOG(ERROR) << "no quant param";
    return RET_ERROR;
  }
  const auto *quant_data = static_cast<const uint8_t *>(input_tensor->MutableData());
  auto *dequant_data = static_cast<float *>(malloc(input_tensor->ElementsNum() * sizeof(float)));
  if (dequant_data == nullptr) {
    MS_LOG(ERROR) << "malloc faile";
    return RET_ERROR;
  }

  if (input_tensor->GetQuantParams().size() != kPerTensor) {
    size_t channels = static_cast<size_t>(input_tensor->Batch());
    if (input_tensor->GetQuantParams().size() != channels) {
      MS_LOG(ERROR) << "Quant param not equal channel num " << input_tensor->GetQuantParams().size() << channels;
      return RET_ERROR;
    }
    size_t per_channel_size = input_tensor->ElementsNum() / channels;
    auto quant_param = input_tensor->GetQuantParams();
    for (size_t i = 0; i < channels; i++) {
      auto param = quant_param.at(i);
      auto scale = param.scale;
      auto zero_point = param.zeroPoint;
      for (size_t j = 0; j < per_channel_size; j++) {
        dequant_data[per_channel_size * i + j] =
          static_cast<float>((quant_data[per_channel_size * i + j] - zero_point) * scale);
      }
    }
  } else {
    auto quant_param = input_tensor->GetQuantParams();
    auto param = quant_param.front();
    auto scale = param.scale;
    auto zero_point = param.zeroPoint;
    for (int64_t j = 0; j < input_tensor->ElementsNum(); j++) {
      dequant_data[j] = static_cast<float>((quant_data[j] - zero_point) * scale);
    }
  }
  input_tensor->SetData(dequant_data);
  return RET_OK;
}
kernel::LiteKernel *CpuScaleFp32KernelCreator(const std::vector<lite::Tensor *> &inputs,
                                              const std::vector<lite::Tensor *> &outputs, OpParameter *opParameter,
                                              const lite::Context *ctx, const kernel::KernelKey &desc,
                                              const mindspore::lite::PrimitiveC *primitive) {
  MS_ASSERT(desc.type == schema::PrimitiveType_Scale);
  auto *weight_tensor = inputs.at(kWeightIndex);
  auto *restore_data = weight_tensor->MutableData();
  if (primitive->GetQuantType() == schema::QuantType_WeightQuant) {
    RestoreScaleWeight(inputs.at(kWeightIndex));
  }
  if (opParameter == nullptr) {
    MS_LOG(ERROR) << "opParameter is nullptr";
    return nullptr;
  }
  auto *kernel = new (std::nothrow) ScaleCPUKernel(opParameter, inputs, outputs, ctx, primitive);
  if (kernel == nullptr) {
    MS_LOG(ERROR) << "New kernel fails.";
    return nullptr;
  }

  auto ret = kernel->Init();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Init kernel failed, name: " << opParameter->name_ << ", type: "
                  << schema::EnumNamePrimitiveType(static_cast<schema::PrimitiveType>(opParameter->type_));
    delete kernel;
    return nullptr;
  }
  if (primitive->GetQuantType() == schema::QuantType_WeightQuant) {
    weight_tensor->FreeData();
    weight_tensor->SetData(restore_data);
  }
  return kernel;
}

REG_KERNEL(kCPU, kNumberTypeFloat32, PrimitiveType_Scale, CpuScaleFp32KernelCreator)
}  // namespace mindspore::kernel
