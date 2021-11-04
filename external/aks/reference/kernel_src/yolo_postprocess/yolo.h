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

#ifndef _YOLO_H_
#define _YOLO_H_

extern "C" {

struct Array {
  int height;
  int width;
  int channels;
  float* data;
};

int yolov2_postproc(Array* output_tensors, int nArrays, const float* biases,
                    int net_h, int net_w, int classes, int anchor_cnt,
                    int img_height, int img_width, float conf_thresh,
                    float iou_thresh, int batch_idx, float** retboxes);

int yolov3_postproc(Array* output_tensors, int nArrays, const float* biases,
                    int net_h, int net_w, int classes, int anchorCnt,
                    int img_height, int img_width, float conf_thresh,
                    float iou_thresh, int batch_idx, float** retboxes);

void clearBuffer(float* buf);

}  // extern "C"
#endif
