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
#include "mindspore/core/utils/log_adapter.h"
#include "common/common_test.h"
#include "mindspore/lite/src/runtime/kernel/arm/fp32/roi_pooling.h"
#include "src/kernel_registry.h"
#include "src/lite_kernel.h"

namespace mindspore {
class TestROIPoolingFp32 : public mindspore::Common {
 public:
  TestROIPoolingFp32() {}
};

int ROIPoolingTestInit(std::vector<lite::tensor::Tensor *> *inputs_, std::vector<lite::tensor::Tensor *> *outputs_,
                       float *a_ptr, float *b_ptr, std::vector<int> a_shape, std::vector<int> b_shape,
                       std::vector<int> c_shape) {
  auto in_t =
    new lite::tensor::Tensor(kNumberTypeFloat, a_shape, schema::Format_NHWC, static_cast<schema::NodeType>(1));
  in_t->MallocData();
  memcpy(in_t->Data(), a_ptr, sizeof(float) * in_t->ElementsNum());
  inputs_->push_back(in_t);

  auto roi_t =
    new lite::tensor::Tensor(kNumberTypeFloat, b_shape, schema::Format_NHWC, static_cast<schema::NodeType>(1));
  roi_t->MallocData();
  memcpy(roi_t->Data(), b_ptr, sizeof(float) * roi_t->ElementsNum());
  inputs_->push_back(roi_t);

  auto out_t =
    new lite::tensor::Tensor(kNumberTypeFloat, c_shape, schema::Format_NHWC, static_cast<schema::NodeType>(1));
  out_t->MallocData();
  outputs_->push_back(out_t);

  return out_t->ElementsNum();
}

TEST_F(TestROIPoolingFp32, Simple) {
  std::vector<lite::tensor::Tensor *> inputs_;
  std::vector<lite::tensor::Tensor *> outputs_;
  auto param = new ROIPoolingParameter();
  param->scale_ = 1;
  param->pooledW_ = 2;
  param->pooledH_ = 2;
  float a[] = {1, 2, 3, 4, 5, 11, 12, 13, 14, 15, 21, 22, 23, 24, 25, 31, 32, 33, 34, 35,
               1, 2, 3, 4, 5, 11, 12, 13, 14, 15, 21, 22, 23, 24, 25, 31, 32, 33, 34, 35};
  float b[] = {0, 1, 1, 3, 4, 1, 1, 1, 3, 4};
  std::vector<int> a_shape = {2, 4, 5, 1};
  std::vector<int> b_shape = {2, 5};
  std::vector<int> c_shape = {2, 2, 2, 1};
  int total_size = ROIPoolingTestInit(&inputs_, &outputs_, a, b, a_shape, b_shape, c_shape);
  auto ctx = new lite::Context;
  ctx->thread_num_ = 1;
  kernel::ROIPoolingCPUKernel *op =
    new kernel::ROIPoolingCPUKernel(reinterpret_cast<OpParameter *>(param), inputs_, outputs_, ctx, nullptr);
  op->Init();
  op->Run();
  float correct[] = {23, 25, 33, 35, 23, 25, 33, 35};
  float *output = reinterpret_cast<float *>(outputs_[0]->Data());
  for (int i = 0; i < 8; ++i) printf("%f ", output[i]);
  printf("\n");
  CompareOutputData(reinterpret_cast<float *>(outputs_[0]->Data()), correct, total_size, 0.0001);
  delete op;
  for (auto t : inputs_) delete t;
  for (auto t : outputs_) delete t;
}
}  // namespace mindspore