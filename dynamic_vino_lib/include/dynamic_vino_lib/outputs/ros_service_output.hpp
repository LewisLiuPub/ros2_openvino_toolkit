// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief A header file with declaration for RosTopicOutput Class
 * @file ros_topic_output.h
 */

#ifndef DYNAMIC_VINO_LIB__OUTPUTS__ROS_SERVICE_OUTPUT_HPP_
#define DYNAMIC_VINO_LIB__OUTPUTS__ROS_SERVICE_OUTPUT_HPP_

#include <object_msgs/msg/object.hpp>
#include <object_msgs/msg/object_in_box.hpp>
#include <object_msgs/msg/objects_in_boxes.hpp>
#include <people_msgs/msg/emotion.hpp>
#include <people_msgs/msg/emotions_stamped.hpp>
#include <people_msgs/msg/age_gender.hpp>
#include <people_msgs/msg/age_gender_stamped.hpp>
#include <people_msgs/msg/head_pose.hpp>
#include <people_msgs/msg/head_pose_stamped.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/header.hpp>

#include <memory>
#include <string>
#include <vector>

#include "dynamic_vino_lib/inferences/face_detection.hpp"
#include "dynamic_vino_lib/outputs/ros_topic_output.hpp"

namespace Outputs
{
/**
 * @class RosServiceOutput
 * @brief This class handles and publish the detection result for service calling.
 */
class RosServiceOutput : public RosTopicOutput
{
public:
  RosServiceOutput(std::string pipeline_name): pipeline_name_(pipeline_name) {}

  /**
   * @brief Publish all the detected infomations generated by the accept
   * functions with ros topic.
   */
  void handleOutput() override {}
  void clearData() override;

  void setServiceResponse(std::shared_ptr<object_msgs::srv::DetectObject::Response> response);
  void setResponseForFace(std::shared_ptr<object_msgs::srv::DetectObject::Response> response);
  void setServiceResponse(std::shared_ptr<people_msgs::srv::AgeGender::Response> response);
  void setServiceResponse(std::shared_ptr<people_msgs::srv::Emotion::Response> response);
  void setServiceResponse(std::shared_ptr<people_msgs::srv::HeadPose::Response> response);
  void setServiceResponse(std::shared_ptr<people_msgs::srv::People::Response> response);

private:
  const std::string service_name_;
  const std::string pipeline_name_;
};
}  // namespace Outputs
#endif  // DYNAMIC_VINO_LIB__OUTPUTS__ROS_SERVICE_OUTPUT_HPP_
