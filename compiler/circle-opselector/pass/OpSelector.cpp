/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "OpSelector.h"

#include "luci/Service/ChangeOutputs.h"

#include <luci/IR/CircleNode.h>

#include <loco/IR/Graph.h>

#include <oops/UserExn.h>

#include <cassert>
#include <iostream>
#include <map>

namespace
{

luci::CircleNode *find_by_name(loco::Graph *g, const std::string &name)
{
  for (auto node : loco::all_nodes(g))
  {
    auto cnode = loco::must_cast<luci::CircleNode *>(node);
    //if (cnode->name().find(name)!=std::string::npos)
    if (cnode->name()==name)
      return cnode;
  }
  return nullptr;
}

} // namespace

namespace opselector
{

void change_outputs(loco::Graph *graph, const std::vector<std::string> &new_outputs)
{
  std::map<std::string, luci::CircleNode *> named_nodes;

  for (auto &node_name : new_outputs)
  {
    auto node = find_by_name(graph, node_name);
    if (node == nullptr)
    {
      throw oops::UserExn("Change outputs failed: node not found: ", node_name);
    }
    named_nodes[node_name] = node;
    std::cout<<"selected node_names: "<< named_nodes[node_name]->name()<<std::endl;
  }

  // TODO Find the way to cut modules
  for (uint32_t out = 0; out < graph->outputs()->size(); ++out)
  {
    auto output = luci::output_node(graph, out); // output is CircleOutput
    assert(output != nullptr);

    auto node_name = new_outputs.at(out);
    auto node = named_nodes[node_name];
    assert(node != nullptr);

    output->from(node);

    // update GraphOutput shape, dtype to node
    auto graph_out = graph->outputs()->at(out);
    auto output_shape = std::make_unique<loco::TensorShape>();

    output_shape->rank(node->rank());
    for (uint32_t r = 0; r < node->rank(); ++r)
    {
      if (node->dim(r).known())
        output_shape->dim(r).set(node->dim(r).value());
      else
        output_shape->dim(r).unset();
    }
    graph_out->shape(std::move(output_shape));
    graph_out->dtype(node->dtype());
  }
}

bool OpSelector::select_nodes_by_name(luci::Module* module, const std::vector<std::string> &by_name)
{ 
  auto graph = module->graph(0);
  opselector::change_outputs(graph, by_name);

  return false; // TODO Use this return value or Change return type void.
}

} // namespace opselector