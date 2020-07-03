/**
 * Copyright 2019 Huawei Technologies Co., Ltd
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
#ifndef MINDSPORE_CCSRC_SESSION_KERNEL_GRAPH_H
#define MINDSPORE_CCSRC_SESSION_KERNEL_GRAPH_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <string>
#include <queue>
#include <map>
#include <set>
#include <unordered_set>
#include "ir/func_graph.h"
#include "ir/anf.h"
#include "utils/graph_utils.h"
#include "utils/contract.h"
#include "device/kernel_info.h"

namespace mindspore {
namespace session {
using AnfWithOutIndex = std::pair<AnfNodePtr, size_t>;
class KernelGraph : public FuncGraph {
 public:
  KernelGraph() : graph_id_(0), start_label_(nullptr), end_goto_(nullptr), null_output_(false) {
    inputs_ = std::make_shared<std::vector<AnfNodePtr>>();
    execution_order_ = {};
    executable_ = true;
    summary_node_exist_ = false;
    stream_distinction_label_ = kInvalidDistincLabel;
  }
  ~KernelGraph() override;

  MS_DECLARE_PARENT(KernelGraph, FuncGraph);

  const std::vector<AnfNodePtr> &inputs() const;
  std::vector<AnfNodePtr> *MutableInputs() const { return inputs_.get(); }
  std::vector<AnfNodePtr> outputs() const;
  CNodePtr NewCNode(const std::vector<AnfNodePtr> &inputs) override;
  void CreateKernelInfoFromNewParameter(const CNodePtr &cnode);
  CNodePtr NewCNode(const CNodePtr &cnode);
  ParameterPtr NewParameter(const ParameterPtr &parameter = nullptr);
  ValueNodePtr NewValueNode(const ValueNodePtr &value_node = nullptr);
  std::vector<AnfNodePtr> SplitTupleValueNodeToNodeList(const ValueNodePtr &value_node);
  void set_execution_order(const std::vector<CNodePtr> &order) { execution_order_ = order; }
  const std::vector<CNodePtr> &execution_order() const { return execution_order_; }
  void SetExecOrderByDefault();
  uint32_t graph_id() const { return graph_id_; }
  void set_graph_id(uint32_t graph_id) { graph_id_ = graph_id; }

  // and a new front to backend anf relation to maop
  void FrontBackendlMapAdd(const AnfNodePtr &front_anf, const AnfNodePtr &backend_anf);
  // replace old backend anf with new backend anf
  void FrontBackendlMapUpdate(const AnfNodePtr &old_backend_anf, const AnfNodePtr &new_backend_anf);
  // get backend anf by front anf
  AnfNodePtr GetBackendAnfByFrontAnf(const AnfNodePtr &front_anf);
  // check backend node whether exist in map
  bool BackendNodeExistInFrontBackendMap(const AnfNodePtr &backend_anf);
  // get value node by tensor
  ValueNodePtr GetValueNodeByTensor(const tensor::TensorPtr &tensor);
  // add value node tensor relation map
  void TensorValueNodeMapAdd(const tensor::TensorPtr &tensor, const ValueNodePtr &value_node);
  // get all value nodes of graph
  const std::unordered_set<ValueNodePtr> graph_value_nodes() const { return graph_value_nodes_; }
  // add value node to graph
  void AddValueNodeToGraph(const ValueNodePtr &value_node);
  // ref output is in map
  bool IsInRefOutputMap(const AnfWithOutIndex &pair) const;
  // get ref correspond pairs
  AnfWithOutIndex GetRefCorrespondOutput(const AnfWithOutIndex &out_pair) const;
  // add ref correspond pairs
  void AddRefCorrespondPairs(const AnfWithOutIndex &final_pair, const AnfWithOutIndex &origin_pair);
  // get map
  std::map<AnfWithOutIndex, AnfWithOutIndex> GetRefMap() const { return ref_out_in_map_; }
  // checkout whether loop exist in graph
  void CheckLoop();
  // check whether graph is executable
  bool executable() const { return executable_; }
  // set executable of graph
  void set_executable(bool executable) { executable_ = executable; }
  // set summary_node of graph
  void set_summary_node_exist(bool summary_node_exist) { summary_node_exist_ = summary_node_exist; }
  // check whether exist summary node in graph
  bool summary_node_exist() const { return summary_node_exist_; }
  // set invalid inputs for control sink
  std::vector<bool> *MutableValidInputs() { return &valid_inputs_; }
  std::vector<bool> valid_inputs() const { return valid_inputs_; }
  // replace node in graph
  void ReplaceNode(NotNull<AnfNodePtr> old_anf_node, NotNull<AnfNodePtr> new_anf_node);
  // set stream label of graph
  void set_stream_distinction_label(uint32_t stream_label) { stream_distinction_label_ = stream_label; }
  // get stream label of graph
  uint32_t stream_distinction_label() { return stream_distinction_label_; }
  // refresh execute kernel stream label
  void UpdateExecuteKernelStreamLabel();
  // calculate the leaf graph order of root graph
  std::vector<std::shared_ptr<KernelGraph>> GetLeafGraphOrder();
  // the child graph of current graph
  const std::vector<std::shared_ptr<KernelGraph>> &child_graph_order() const { return child_graph_order_; }
  void set_child_graph_order(const std::vector<std::shared_ptr<KernelGraph>> &order) { child_graph_order_ = order; }
  // checkout whether current graph is leaf graph
  bool IsLeafGraph() const;

  // set input_tensors pointer of control parameter
  void set_input_ctrl_tensors(const std::shared_ptr<std::vector<tensor::TensorPtr>> &input_tensors_ptr) {
    input_ctrl_tensors_ = input_tensors_ptr;
  }
  // get input_tensors pointer of control parameter
  std::shared_ptr<std::vector<tensor::TensorPtr>> input_ctrl_tensors() const { return input_ctrl_tensors_; }
  // get parent kernel graph
  std::shared_ptr<KernelGraph> parent_graph() const { return parent_graph_; }
  // set parent kernel graph
  void set_parent_graph(const std::shared_ptr<KernelGraph> &parent_graph) { parent_graph_ = parent_graph; }
  // find anf node in graph
  std::vector<CNodePtr> FindNodeByPrimitive(const PrimitivePtr &primitive) const;
  // get real inputs
  const std::vector<std::pair<AnfNodePtr, std::vector<AnfNodePtr>>> &real_inputs() const { return real_inputs_; }
  void SetRealInput(const AnfNodePtr &parameter, const AnfNodePtr &arg);
  // used to dump ir
  std::string ToString() const override;
  // update the real input if the node is a call
  void UpdateCallRealInput();

  void set_start_label(const CNodePtr &start_label) { start_label_ = start_label; }
  CNodePtr get_start_label() { return start_label_; }
  void set_end_goto(const CNodePtr &end_goto) { end_goto_ = end_goto; }
  CNodePtr get_end_goto() { return end_goto_; }
  bool get_output_null() { return null_output_; }
  void set_output_null(bool is_output_null) { null_output_ = is_output_null; }
  void PrintGraphExecuteOrder() const;
  const std::map<std::string, std::pair<AnfNodePtr, int>> &summary_nodes() const { return summary_nodes_; }
  void set_summary_nodes(const std::map<std::string, std::pair<AnfNodePtr, int>> &nodes) { summary_nodes_ = nodes; }
  void AddInternalOutput(const AnfNodePtr &front_node, const AnfNodePtr &node);
  void ReplaceInternalOutput(const AnfNodePtr &node, const AnfNodePtr &new_node);
  AnfNodePtr GetInternalOutputByFrontNode(const AnfNodePtr &front_node) const;
  bool IsInternalOutput(const AnfNodePtr &node) const;
  AnfNodePtr GetFrontNodeByInternalOutput(const AnfNodePtr &node) const;
  void AddFinalOutputKernel(const AnfNodePtr &node);
  bool IsFinalOutputKernel(const AnfNodePtr &node) const;

 private:
  // remove value node form graph
  bool RemoveValueNodeFromGraph(const ValueNodePtr &value_node);
  void VisitNodeDescendants(const AnfNodePtr &node, std::queue<AnfNodePtr> *visit_queue,
                            std::unordered_set<AnfNodePtr> *visited_nodes);
  // update node edge list
  void UpdateNodeEdgeList(std::queue<AnfNodePtr> *seed_nodes);
  // add node depend edge by data edge or control depend
  void AddDependEdge(const AnfNodePtr &node, const AnfNodePtr &input, size_t depend_edge_num);
  // handle control depend
  std::vector<AnfNodePtr> GetOutputNodes(const AnfNodePtr &node);
  bool HandleControlDependNode(const AnfNodePtr &node, std::queue<AnfNodePtr> *que,
                               std::unordered_set<AnfNodePtr> *visited_nodes);
  void UpdateControlDependRelations(const std::vector<AnfNodePtr> &depends);

  std::shared_ptr<std::vector<AnfNodePtr>> inputs_;
  std::vector<CNodePtr> execution_order_;
  uint32_t graph_id_;
  uint32_t stream_distinction_label_;

  // record map bettween front anf and backend anf,use two map implement bidirectional map
  std::unordered_map<AnfNodePtr, AnfNodePtr> front_backend_anf_map_;
  std::unordered_map<AnfNodePtr, AnfNodePtr> backend_front_anf_map_;
  // there may be a tensor from ME backend ,a value ndoe will be create according the tensor,map record
  std::unordered_map<tensor::TensorPtr, ValueNodePtr> tensor_to_value_node_map_;
  // include all value nodes
  std::unordered_set<ValueNodePtr> graph_value_nodes_;
  std::unordered_map<AnfNodePtr, size_t> node_input_num_;
  std::unordered_map<AnfNodePtr, std::vector<std::pair<AnfNodePtr, size_t>>> node_input_edges_;
  // record map between ref final output anf with index and ref origin input with index
  std::map<AnfWithOutIndex, AnfWithOutIndex> ref_out_in_map_;
  std::unordered_map<AnfNodePtr, std::vector<std::pair<AnfNodePtr, size_t>>> node_output_edges_;
  std::map<std::string, std::pair<AnfNodePtr, int>> summary_nodes_;
  // graph needn't execute
  bool executable_;
  // exist summary node in graph
  bool summary_node_exist_;
  // valid inputs
  std::vector<bool> valid_inputs_;

  // new members for control sink process
  // all child grahs refers to partial node
  std::map<AnfNodePtr, std::shared_ptr<KernelGraph>> node_to_child_graphs_;
  // child graph execute order in root graph
  std::vector<std::shared_ptr<KernelGraph>> child_graph_order_;

  // input_tensors of control parameter
  std::shared_ptr<std::vector<tensor::TensorPtr>> input_ctrl_tensors_;

  // parameter graph
  std::shared_ptr<KernelGraph> parent_graph_;
  // record real parameters,inputs_ is the formal parameters
  std::vector<std::pair<AnfNodePtr, std::vector<AnfNodePtr>>> real_inputs_;

  CNodePtr start_label_;
  CNodePtr end_goto_;
  bool null_output_;
  std::unordered_map<AnfNodePtr, AnfNodePtr> front_to_internal_outputs_map_;
  std::unordered_map<AnfNodePtr, AnfNodePtr> internal_outputs_to_front_map_;
  std::set<AnfNodePtr> final_output_kernels_;
};
}  // namespace session
using KernelGraphPtr = std::shared_ptr<session::KernelGraph>;
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_SESSION_KERNEL_GRAPH_H
