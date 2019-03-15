/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief a header file with declaration of PipelineFilters class
 * @file pipeline_filters.hpp
 */
#ifndef DYNAMIC_VINO_LIB__PIPELINE_FILTERS_HPP_
#define DYNAMIC_VINO_LIB__PIPELINE_FILTERS_HPP_

#include <vino_param_lib/param_manager.hpp>
#include "opencv2/opencv.hpp"

/**
 * @class PipelineFilters
 * @brief This class is a pipeline parameter management that stores parameters
 * of a given pipeline
 */
class PipelineFilters {
 public:
  PipelineFilters();
  PipelineParams(const Params::ParamManager::PipelineRawData& params);
  void add(const Params::ParamManager::FilterRawData& params);
  virtual std::shared_ptr<BaseFilterSolver> getFilterSolver(const std::string& type);
  std::vector<std::shared_ptr<BaseFilterSolver>>
    findPipelineFilters(const std::string& input, const std::string& output);
public:
  struct PipelineFilterNative{
    std::string input;
    std::string output;
    std::vector<std::shared_ptr<BaseFilterSolver>> filter_solvers;
  };

 private:
  std::vector<PipelineFilterNative> pipeline_filters_;

};

#endif  // DYNAMIC_VINO_LIB__PIPELINE_FILTERS_HPP_
