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
 * @brief a header file with declaration of ObjectDetectionModel class
 * @file object_detection_model.cpp
 */
#include <string>
#include "dynamic_vino_lib/models/object_detection_yolov2_model.hpp"
#include "dynamic_vino_lib/slog.hpp"
// Validated Object Detection Network
Models::ObjectDetectionYolov2Model::ObjectDetectionYolov2Model(
  const std::string & model_loc, int input_num,
  int output_num, int max_batch_size)
: ObjectDetectionModel(model_loc, input_num, output_num, max_batch_size)
{
}

void Models::ObjectDetectionYolov2Model::setLayerProperty(InferenceEngine::CNNNetReader::Ptr net_reader)
{
  // set input property
  InferenceEngine::InputsDataMap input_info_map(net_reader->getNetwork().getInputsInfo());
  InferenceEngine::InputInfo::Ptr input_info = input_info_map.begin()->second;
  input_info->setPrecision(InferenceEngine::Precision::FP32);
  input_info->getInputData()->setLayout(InferenceEngine::Layout::NCHW);

  // set output property
  InferenceEngine::OutputsDataMap output_info_map(net_reader->getNetwork().getOutputsInfo());
  InferenceEngine::DataPtr & output_data_ptr = output_info_map.begin()->second;
  output_data_ptr->setPrecision(InferenceEngine::Precision::FP32);

  // set input and output layer name
  input_ = input_info_map.begin()->first;
  output_ = output_info_map.begin()->first;
}

void Models::ObjectDetectionYolov2Model::checkLayerProperty(
  const InferenceEngine::CNNNetReader::Ptr & net_reader)
{
  slog::info << "Checking Object Detection outputs" << slog::endl;
  InferenceEngine::OutputsDataMap output_info_map(net_reader->getNetwork().getOutputsInfo());
  slog::info << "Checking Object Detection outputs ..." << slog::endl;
  if (output_info_map.size() != 1) {
    throw std::logic_error("This sample accepts networks having only one output");
  }
  InferenceEngine::DataPtr & output_data_ptr = output_info_map.begin()->second;
  output_ = output_info_map.begin()->first;
  slog::info << "Checking Object Detection output ... Name=" << output_ << slog::endl;

  const InferenceEngine::CNNLayerPtr output_layer =
    net_reader->getNetwork().getLayerByName(output_.c_str());
  // output layer should have attribute called num_classes
  slog::info << "Checking Object Detection num_classes" << slog::endl;
  if (output_layer->params.find("classes") == output_layer->params.end()) {
    throw std::logic_error("Object Detection network output layer (" + output_ +
            ") should have classes integer attribute");
  }
  // class number should be equal to size of label vector
  // if network has default "background" class, fake is used
  const int num_classes = output_layer->GetParamAsInt("classes");

  slog::info << "Checking Object Detection output ... num_classes=" << num_classes << slog::endl;
  if (getLabels().size() != num_classes) {
    if (getLabels().size() == (num_classes - 1)) {
      getLabels().insert(getLabels().begin(), "fake");
    } else {
      getLabels().clear();
    }
  }
  // last dimension of output layer should be 7
  const InferenceEngine::SizeVector output_dims = output_data_ptr->getTensorDesc().getDims();
  max_proposal_count_ = static_cast<int>(output_dims[2]);
  slog::info << "max proposal count is: " << max_proposal_count_ << slog::endl;
  object_size_ = static_cast<int>(output_dims[3]);
  if (object_size_ != 33) {
    throw std::logic_error("Object Detection network output layer should have 33 as a last "
            "dimension");
  }
  if (output_dims.size() != 2) {
    throw std::logic_error("Object Detection network output dimensions not compatible shoulld be "
            "2, "
            "but was " +
            std::to_string(output_dims.size()));
  }
}

const std::string Models::ObjectDetectionYolov2Model::getModelName() const
{
  return "Object Detection Yolo v2";
}

bool Models::ObjectDetectionYolov2Model::enqueue(
  const std::shared_ptr<Engines::Engine>& engine,
  const cv::Mat & frame,
  const cv::Rect & input_frame_loc) const
{
  if (width_ == 0 && height_ == 0)
  {
    width_ = frame.cols;
    height_ = frame.rows;
  }

  if (!matToBlob(frame, input_frame_loc, 1, 0, engine))
  {
    return false;
  }
  return true;
}

bool Models::ObjectDetectionYolov2Model::matToBlob(
    const cv::Mat& orig_image, const cv::Rect&, float scale_factor, 
    int batch_index, const std::shared_ptr<Engines::Engine>& engine)
{
  if(engine == nullptr){
    slog::err << "A frame is trying to be enqueued in a NULL Engine." << slog::endl;
    return false;
  }

  std::string input_name = getInputName();
  InferenceEngine::Blob::Ptr input_blob =
      engine->getRequest()->GetBlob(input_name);

  InferenceEngine::SizeVector blob_size = input_blob->getTensorDesc().getDims();
  const int width = blob_size[3];
  const int height = blob_size[2];
  const int channels = blob_size[1];
  float* blob_data = input_blob->buffer().as<float*>();

  
  int dx = 0;
  int dy = 0;
  int srcw = 0;
  int srch = 0;

  int IH = height;
  int IW = width;

  cv::Mat image = orig_image.clone();
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

  image.convertTo(image, CV_32F, 1.0/255.0, 0);
  srcw = image.size().width;
  srch = image.size().height;

  cv::Mat resizedImg (IH, IW, CV_32FC3);
  resizedImg = cv::Scalar(0.5, 0.5, 0.5);
  int imw = image.size().width;
  int imh = image.size().height;
  float resize_ratio = (float)IH / (float)std::max(imw, imh);
  cv::resize(image, image, cv::Size(imw*resize_ratio, imh*resize_ratio));

  int new_w = imw;
  int new_h = imh;
  if (((float)IW/imw) < ((float)IH/imh)) {
    new_w = IW;
    new_h = (imh * IW)/imw;
  } else {
    new_h = IH;
    new_w = (imw * IW)/imh;
  }
  dx = (IW-new_w)/2;
  dy = (IH-new_h)/2;

  imh = image.size().height;
  imw = image.size().width;

  for(int row = 0; row < imh; row ++)
  {
    for(int col = 0; col < imw; col ++)
    {
      for(int ch = 0; ch < 3; ch ++)
      {
        resizedImg.at<cv::Vec3f>(dy + row, dx + col)[ch] = image.at<cv::Vec3f>(row, col)[ch];
      }
    }
  }

  for (int c = 0; c < channels; c++)
  {
    for (int h = 0; h < height; h++)
    {
      for (int w = 0; w < width; w++)
      {
        blob_data[c * width * height + h*width + w] = resizedImg.at<cv::Vec3f>(h, w)[c];
      }
    }
  }

  setFrameSize(srcw, srch);
  return true;
}

bool Models::ObjectDetectionYolov2Model::fetchResults(
  const std::shared_ptr<Engines::Engine>& engine,
  std::vector<dynamic_vino_lib::ObjectDetectionResult>& result,
  const bool& enable_roi_constraint)
{

  if(engine == nullptr){
    slog::err << "Trying to fetch results from <null> Engines." << slog::endl;
    return false;
  }

  InferenceEngine::InferRequest::Ptr request = engine->getRequest();

  std::string output = getOutputName();
  std::vector<std::string>& labels = getLabels();

  const float* detections = request->GetBlob(output)->buffer().as<InferenceEngine::PrecisionTrait<InferenceEngine::Precision::FP32>::value_type *>();
  InferenceEngine::CNNLayerPtr layer = getLayer();
  InferenceEngine::InputInfo::Ptr input_info = getInputInfo();

  int input_height = input_info->getTensorDesc().getDims()[2];
  int input_width = input_info->getTensorDesc().getDims()[3];

  // --------------------------- Validating output parameters -------------------------------------
  if (layer != nullptr && layer->type != "RegionYolo"){
      throw std::runtime_error("Invalid output type: " + layer->type + ". RegionYolo expected");
  }
  // --------------------------- Extracting layer parameters -------------------------------------
  const int num = layer->GetParamAsInt("num");
  const int coords = layer->GetParamAsInt("coords");
  const int classes = layer->GetParamAsInt("classes");

  const int out_blob_h = layer->input()->dims[0];

  std::vector<float> anchors = {
      0.572730, 0.677385,
      1.874460, 2.062530,
      3.338430, 5.474340,
      7.882820, 3.527780,
      9.770520, 9.168280
  };
  float threshold = 0.5;
  auto side = out_blob_h;

  auto side_square = side * side;

  // --------------------------- Parsing YOLO Region output -------------------------------------
  std::vector<Result> raw_results;
  for (int i = 0; i < side_square; ++i) {
    int row = i / side;
    int col = i % side;

    for (int n = 0; n < num; ++n) {
      int obj_index = getEntryIndex(side, coords, classes, n * side * side + i, coords);
      int box_index = getEntryIndex(side, coords, classes, n * side * side + i, 0);

      float scale = detections[obj_index];

      if (scale < threshold)
          continue;

      float x = (col + detections[box_index + 0 * side_square]) / side * input_width;
      float y = (row + detections[box_index + 1 * side_square]) / side * input_height;
      float height = std::exp(detections[box_index + 3 * side_square]) * anchors[2 * n + 1] / side * input_height;
      float width  = std::exp(detections[box_index + 2 * side_square]) * anchors[2 * n] / side * input_width;

      for (int j = 0; j < classes; ++j) {

        int class_index = getEntryIndex(side, coords, classes, n * side_square + i, coords + 1 + j);

        float prob = scale * detections[class_index];
        if (prob < threshold)
          continue;

        float x_min = x - width/2;
        float y_min = y - height/2;

        float x_min_resized = x_min / input_width * frame_size_.width;
        float y_min_resized = y_min / input_height * frame_size_.height;
        float width_resized = width / input_width * frame_size_.width;
        float height_resized = height / input_height * frame_size_.height;

        cv::Rect r(x_min_resized, y_min_resized, width_resized, height_resized);
        Result result(r);
        result.label_ = j;
        result.label_ = labels[j];

        result.confidence_ = prob;
        raw_results.emplace_back(result);
      }
    }
  }

  std::sort(raw_results.begin(), raw_results.end());
  for (unsigned int i = 0; i < raw_results.size(); ++i) {
    if (raw_results[i].confidence_ == 0)
      continue;
      for (unsigned int j = i + 1; j < raw_results.size(); ++j)
      auto iou = ObjectDetection::IntersectionOverUnion(
        raw_results[i].getLocation(), raw_results[j].getLocation());
      if (iou) >= 0.45)
        raw_results[j].confidence_ = 0;
  }

  for (auto &raw_result : raw_results) {
    if (raw_result.getConfidence() < 0.5)
       continue;

    results_.push_back(raw_result);
  }

  raw_results.clear();

  return true;
}

int dynamic_vino_lib::ObjectDetectionYolov2Model::getEntryIndex(int side, int lcoords, int lclasses, int location, int entry)
{
  int n = location / (side * side);
  int loc = location % (side * side);
  return n * side * side * (lcoords + lclasses + 1) + entry * side * side + loc;
}