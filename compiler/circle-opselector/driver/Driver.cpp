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

#include "SinglePass.h"
#include "Function1.h"
// TODO Add new pass headers

#include <foder/FileLoader.h>

#include <luci/Importer.h>
#include <luci/CircleExporter.h>
#include <luci/CircleFileExpContract.h>

#include <arser/arser.h>
#include <vconone/vconone.h>

#include <iostream>
#include <string>
#include <vector>

void print_version(void)
{
  std::cout << "circle-opselector version " << vconone::get_string() << std::endl;
  std::cout << vconone::get_copyright() << std::endl;
}

int entry(int argc, char **argv)
{
  // TODO Add new option names!

  arser::Arser arser("circle-opselector provides selecting operations in circle model");

  arser.add_argument("--version")
    .nargs(0)
    .required(false)
    .default_value(false)
    .help("Show version information and exit")
    .exit_with(print_version);

  // TODO Add new options!

  arser.add_argument("input").nargs(1).type(arser::DataType::STR).help("Input circle model");
  arser.add_argument("output").nargs(1).type(arser::DataType::STR).help("Output circle model");

  // select option
  arser.add_argument("--select")
    .nargs(1)
    .type(arser::DataType::STR)
    .help("Selecte opeartors from the input circle");
  arser.add_argument("--deselect")
    .nargs(1)
    .type(arser::DataType::STR)
    .help("Exclude operators from the input circle");

  try
  {
    arser.parse(argc, argv);
  }
  catch (const std::runtime_error &err)
  {
    std::cout << err.what() << std::endl;
    std::cout << arser;
    return EXIT_FAILURE;
  }

  std::string input_path = arser.get<std::string>("input");
  std::string output_path = arser.get<std::string>("output");

  std::string operators;
  
  if (arser["--select"])
    operators = arser.get<std::string>("--select");
  if (arser["--deselect"])
    operators = arser.get<std::string>("--deselect");

  // Load model from the file
  foder::FileLoader file_loader{input_path};
  std::vector<char> model_data = file_loader.load();

  // Verify flatbuffers
  flatbuffers::Verifier verifier{reinterpret_cast<uint8_t *>(model_data.data()), model_data.size()};
  if (!circle::VerifyModelBuffer(verifier))
  {
    std::cerr << "ERROR: Invalid input file '" << input_path << "'" << std::endl;
    return EXIT_FAILURE;
  }

  const circle::Model *circle_model = circle::GetModel(model_data.data());
  if (circle_model == nullptr)
  {
    std::cerr << "ERROR: Failed to load circle '" << input_path << "'" << std::endl;
    return EXIT_FAILURE;
  }

  // Import from input Circle file
  luci::Importer importer;
  auto module = importer.importModule(circle_model);

  // Enable each pass
  std::vector<std::unique_ptr<opselector::SinglePass>> passes;

  // TODO Add new passes!
  passes.emplace_back(std::make_unique<opselector::Function1>());
  if (operators.size())
  {
    std::cout << operators << std::endl;
  }
    // Run for each passes
  for (auto &pass : passes)
  {
    std::cout << pass->run(module.get()) << std::endl;
  }

  // Export to output Circle file
  luci::CircleExporter exporter;

  luci::CircleFileExpContract contract(module.get(), output_path);

  if (!exporter.invoke(&contract))
  {
    std::cerr << "ERROR: Failed to export '" << output_path << "'" << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}
