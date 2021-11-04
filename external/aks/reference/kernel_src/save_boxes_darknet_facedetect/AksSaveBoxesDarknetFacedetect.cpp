/*
 * Copyright 2019 Xilinx Inc.
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
#include <aks/AksKernelBase.h>
#include <aks/AksNodeParams.h>
#include <aks/AksTensorBuffer.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <vector>

// Box coordinates as (x, y, w, h)
struct XYWH {
  float x, y, w, h;
};

class SaveBoxesDarknetFormat : public AKS::KernelBase {
 private:
  std::string _output_dir;
  int _net_h;
  int _net_w;
  ///. Save the output from postproc kernel in Darknet format for mAP
  ///calculation.
  XYWH _opencvStyleCoords(float img_w, float img_h, float llx, float lly,
                          float urx, float ury);

 public:
  void nodeInit(AKS::NodeParams*);
  int exec_async(std::vector<vart::TensorBuffer*>& in,
                 std::vector<vart::TensorBuffer*>& out,
                 AKS::NodeParams* nodeParams,
                 AKS::DynamicParamValues* dynParams);
};

extern "C" {  /// Add this to make this available for python bindings and

AKS::KernelBase* getKernel(AKS::NodeParams* params) {
  return new SaveBoxesDarknetFormat();
}

}  // externC

void SaveBoxesDarknetFormat::nodeInit(AKS::NodeParams* nodeParams) {
  _output_dir = nodeParams->hasKey<std::string>("output_dir")
                  ? nodeParams->getValue<std::string>("output_dir")
                  : "";
  if (!_output_dir.empty() && !boost::filesystem::exists(_output_dir)) {
    boost::filesystem::create_directory(_output_dir);
  }
  _net_h = nodeParams->_intParams["net_h"];
  _net_w = nodeParams->_intParams["net_w"];
}

void writeOutput(const std::string& filename) {}

int SaveBoxesDarknetFormat::exec_async(std::vector<vart::TensorBuffer*>& in,
                                       std::vector<vart::TensorBuffer*>& out,
                                       AKS::NodeParams* nodeParams,
                                       AKS::DynamicParamValues* dynParams) {
  // Get imgFile name from the full path
  const auto& imagePaths = dynParams->imagePaths;
  std::vector<int>& imgShapes = dynParams->_intVectorParams.at("img_dims");

  auto* boxes = in[0];
  int nboxes = boxes->get_tensor()->get_shape()[0];
  int new_boxes = nboxes / 6;
  const auto* boxptr = reinterpret_cast<float*>(in[0]->data().first);

  auto* outBuf = new AKS::AksTensorBuffer(xir::Tensor::create(
    "facedetect", {new_boxes, 7}, xir::create_data_type<float>()));
  auto* outptr = reinterpret_cast<float*>(outBuf->data().first);

  // Convert boxes to darknet format
  for (int i = 0; i < new_boxes; ++i) {
    float b = boxptr[0];
    float x = boxptr[1];
    float y = boxptr[2];
    float w = boxptr[3];
    float h = boxptr[4];
    float class_id = -1;
    float score = boxptr[5];

    int batch = static_cast<int>(b);
    int img_h = imgShapes[b * 3 + 1];
    int img_w = imgShapes[b * 3 + 2];

    XYWH xywh = _opencvStyleCoords(img_w, img_h, x, y, w, h);

    outptr[0] = b;
    outptr[1] = class_id;
    outptr[2] = score;
    outptr[3] = xywh.x;
    outptr[4] = xywh.y;
    outptr[5] = xywh.w;
    outptr[6] = xywh.h;

    boxptr += 6;
    outptr += 7;
  }

  // Dump the boxes to file if needed
  if (!_output_dir.empty()) {
    boxptr = reinterpret_cast<float*>(outBuf->data().first);
    int boxcnt = 0;
    for (int b = 0; b < imagePaths.size(); ++b) {
      std::vector<std::string> tokens;
      boost::split(tokens, imagePaths[b], boost::is_any_of("/,."));
      auto& imgFile = tokens[tokens.size() - 2];

      // Append output_dir and .txt to get output file
      std::string output_file = _output_dir + "/" + imgFile + ".txt";
      ofstream f(output_file);
      if (!f) {
        std::cerr << "[WARNING] : Couldn't open " << output_file << std::endl;
        std::cerr << "[WARNING] : Check if path is correct" << std::endl;
        return -1;
      }

      while ((boxptr[0] == b) && (boxcnt < nboxes)) {
        float class_id = boxptr[1];
        float score = boxptr[2];
        float x = boxptr[3];
        float y = boxptr[4];
        float w = boxptr[5];
        float h = boxptr[6];

        f << class_id << " " << score << " ";
        f << x << " " << y << " ";
        f << w << " " << h << '\n';
        boxptr += 7;
        ++boxcnt;
      }
      f.close();
    }
  }

  out.push_back(outBuf);
  return -1;
}

XYWH SaveBoxesDarknetFormat::_opencvStyleCoords(float img_w, float img_h,
                                                float x, float y, float w,
                                                float h) {
  float dw = 1.0f / (_net_w);
  float dh = 1.0f / (_net_h);
  x = x * dw * img_w;
  w = w * dw * img_w;
  y = y * dh * img_h;
  h = h * dh * img_h;
  return XYWH{x, y, w, h};
}
