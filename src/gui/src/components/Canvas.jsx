// Copyright 2021 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import React from "react"

class Canvas extends React.Component {
  constructor(props) {
    super(props);
    this.canvasRef = React.createRef();
    this.imageRef = React.createRef();
    this.height = this.props.height;
    this.width = this.props.width;
  }

  componentDidUpdate() {
    const canvas = this.canvasRef.current;
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    if(this.props.imageStr !== ""){
      let image = new Image();
      image.onload = function() {
        ctx.drawImage(image, 0, 0, this.width, this.height, 0, 0, canvas.width, canvas.height);
      };
      image.src = this.props.imageStr;
    } else if(typeof this.props.imageData === 'object' && this.props.imageData !== null) {
      ctx.putImageData(this.props.imageData, 0, 0);
    }
  }

  render() {
    return(
      <canvas ref={this.canvasRef} width={this.width} height={this.height} />
    )
  }
}

class StreamingCanvas extends React.Component {
  constructor(props) {
    super(props);
    this.canvasRef = React.createRef();
    this.imageRef = React.createRef();
    this.height = this.props.height;
    this.width = this.props.width;
    this.labels = this.props.labels;
    this.colors = ['blue', 'red', 'green', 'orange', 'purple', 'deeppink',
    'cyan', 'yellow', 'brown'];
    this.colorsIndex = 0;
  }

  componentDidUpdate() {
    const canvas = this.canvasRef.current;
    const ctx = canvas.getContext("2d");
    if(this.props.imageStr !== null){
      let image = new Image();
      let imageObj = this.props.imageStr; // contains img, labels, height, width
      let labels = imageObj.labels;
      // console.log(labels)
      let numLabels = labels.length;
      let my_canvas = this;

      const x_scale = canvas.width / imageObj.width;
      const y_scale = canvas.height / imageObj.height;

      image.onload = function() {
        ctx.save(); // Freeze redraw
        ctx.drawImage(image, 0, 0, this.width, this.height, 0, 0, canvas.width, canvas.height);
        // my_canvas.colorsIndex = 0;
        for(let index = 0; index < numLabels; index++){
          let fill = labels[index].fill;
          let coords = labels[index].box;

          let label = "";
          let label_index = -1;
          if(!isNaN(labels[index].label)){
            // if it's a number, assume it's a resnet index
            label_index = parseInt(labels[index].label);
            if(label_index >= 0){
              if(my_canvas.labels === null){
                label = "";
                console.log("Attempted to use null labels");
              } else {
                label = my_canvas.labels[label_index];
              }
            } else {
              label = "";
            }
          } else {
            label = labels[index].label;
          }

          if(fill){
            ctx.fillStyle = 'rgba(0,0,0,0.5)';
            ctx.fillRect(coords[0]*x_scale,coords[1]*y_scale,canvas.width,coords[3]*y_scale);
          } else {
            let color_index = label_index >= 0 ? label_index : index;
            ctx.strokeStyle = my_canvas.colors[color_index % my_canvas.colors.length];
            ctx.strokeRect(coords[0]*x_scale,coords[1]*y_scale,coords[2]*x_scale,coords[3]*y_scale);
          }
          ctx.fillStyle = 'rgba(255,255,255,1)';
          ctx.fillText(label, coords[0]*x_scale, (coords[1] + 10)*y_scale);
        }
        ctx.restore(); // now redraw
      };
      image.src = imageObj.img;
    }
  }

  render() {
    return(
      <canvas ref={this.canvasRef} width={this.width} height={this.height} />

    )
  }
}

export {
  Canvas,
  StreamingCanvas
}
