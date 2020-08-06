/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *conv_activation_fusion.h
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_LITE_SRC_PASS_FUSION_CONV_ACTIVATION_FUSION_H_
#define MINDSPORE_LITE_SRC_PASS_FUSION_CONV_ACTIVATION_FUSION_H_

#include <string>
#include "src/gllo/common/optimizer.h"

namespace mindspore {
namespace opt {
class ConvActivationFusion : public PatternProcessPass {
 public:
  explicit ConvActivationFusion(bool multigraph = true, const std::string &name = "conv_activation_fusion",
                                schema::PrimitiveType primitive = schema::PrimitiveType_LeakyReLU,
                                schema::ActivationType activation = schema::ActivationType_LEAKY_RELU) : primitive_type(
      primitive), activation_type(activation), PatternProcessPass(name, multigraph) {}
  ~ConvActivationFusion() override = default;
  const BaseRef DefinePattern() const override;
  const AnfNodePtr Process(const FuncGraphPtr &, const AnfNodePtr &, const EquivPtr &) const override;
  schema::PrimitiveType primitive_type;
  schema::ActivationType activation_type;
};
}  // namespace opt
}  // namespace mindspore
#endif  // MINDSPORE_LITE_SRC_PASS_FUSION_CONV_ACTIVATION_FUSION_H_
