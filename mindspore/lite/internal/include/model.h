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
#ifndef MINDSPORE_LITE_INTERNAL_INCLUDE_MODEL_H
#define MINDSPORE_LITE_INTERNAL_INCLUDE_MODEL_H
#include "internal/include/lite_utils.h"
#include "nnacl/op_base.h"

using PrimitiveC = OpParameter;
enum NodeType {
  NodeType_ValueNode = 0,
  NodeType_Parameter = 1,
  NodeType_CNode = 2,
  NodeType_MIN = NodeType_ValueNode,
  NodeType_MAX = NodeType_CNode
};

typedef struct Node {
  String name_;
  NodeType node_type_;
  PrimitiveC *primitive_;
  Uint32Vector input_indices_;
  Uint32Vector output_indices_;
} Node;

typedef struct Model {
  String name_;
  String version_;
  TensorPtrVector all_tensors_;
  Uint32Vector input_indices_;
  Uint32Vector output_indices_;
  NodePtrVector nodes_;
  char *buf;

  /// \brief Static method to create a Model pointer.
  ///
  /// \param[in] model_buf Define the buffer read from a model file.
  /// \param[in] size Define bytes number of model buffer.
  ///
  /// \return Pointer of MindSpore Lite Model.
  static Model *Import(const char *model_buf, size_t size);

  /// \brief Free all the temporary buffer
  void Free();
} Model;

#endif  // MINDSPORE_LITE_INTERNAL_INCLUDE_MODEL_H
