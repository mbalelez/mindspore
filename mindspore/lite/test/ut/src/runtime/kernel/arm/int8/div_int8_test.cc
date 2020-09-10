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

#include <iostream>
#include <memory>
#include "schema/inner/model_generated.h"
#include "common/common_test.h"
#include "mindspore/lite/src/runtime/kernel/arm/int8/div_int8.h"
#include "mindspore/lite/src/kernel_registry.h"
#include "mindspore/lite/include/context.h"

namespace mindspore {
class TestDivInt8 : public mindspore::CommonTest {
 public:
  TestDivInt8() {}
};

TEST_F(TestDivInt8, DivInt8) {
  lite::Tensor in_tensor0(kNumberTypeInt8, {1, 1, 2, 5});
  lite::Tensor in_tensor1(kNumberTypeInt8, {1, 1, 2, 5});
  lite::Tensor out_tensor(kNumberTypeInt8, {1, 1, 2, 5});

  int8_t input_data0[] = {105, 35, -27, 0, -63, 99, 16, 45, 67, -49};
  int8_t input_data1[] = {126, -38, -115, 106, -98, 119, 103, 81, -114, 68};
  int8_t output_data[10] = {0};
  in_tensor0.SetData(input_data0);
  in_tensor1.SetData(input_data1);
  out_tensor.SetData(output_data);

  const lite::QuantArg quant_in0 = {0.00784314f, 0};  // -1.0--1.0 -> 0--255
  const lite::QuantArg quant_in1 = {0.00784314f, 0};
  const lite::QuantArg quant_out = {0.00784314f, 0};
  in_tensor0.AddQuantParam(quant_in0);
  in_tensor1.AddQuantParam(quant_in1);
  out_tensor.AddQuantParam(quant_out);

  std::vector<lite::Tensor *> inputs = {&in_tensor0, &in_tensor1};
  std::vector<lite::Tensor *> outputs = {&out_tensor};

  OpParameter parameter = {};
  kernel::KernelKey desc = {kernel::KERNEL_ARCH::kCPU, kNumberTypeInt8, schema::PrimitiveType_Div};

  auto creator = lite::KernelRegistry::GetInstance()->GetCreator(desc);
  ASSERT_NE(creator, nullptr);

  auto ctx = std::make_shared<lite::Context>();
  auto kernel = creator(inputs, outputs, reinterpret_cast<OpParameter *>(&parameter), ctx.get(), desc, nullptr);
  ASSERT_NE(kernel, nullptr);

  auto ret = kernel->Run();
  EXPECT_EQ(0, ret);

  int8_t expect0[10] = {106, -117, 30, 0, 82, 106, 20, 71, -75, -92};
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(output_data[i], expect0[i]);
  }

  in_tensor0.SetData(nullptr);
  in_tensor1.SetData(nullptr);
  out_tensor.SetData(nullptr);
}
}  // namespace mindspore
