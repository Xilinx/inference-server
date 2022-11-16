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

import Button from "react-bootstrap/Button"
import Form from "react-bootstrap/Form"
import {useState, useRef} from "react"

import {getWsURL, postLoad} from "../api/predict_api_v2"
import Queue from "../helpers/Queue"
import {StreamingCanvas} from "../components/Canvas"
import {useMutation} from "react-query"

import {default as resnet_labels} from "data/resnet50"
import {default as cityscapes_labels} from "data/cityscapes"

import { makeStyles } from '@material-ui/core/styles';
import GridList from '@material-ui/core/GridList';
import GridListTile from '@material-ui/core/GridListTile';

const useStyles = makeStyles((theme) => ({
  root: {
    display: 'flex',
    flexWrap: 'wrap',
    justifyContent: 'space-around',
    overflow: 'hidden',
    backgroundColor: theme.palette.background.paper,
  },
  // gridList: {
  //   width: 1000,
  //   height: 1000,
  // },
  // titleBar: {
  //   background:
  //     'linear-gradient(to top, rgba(0,0,0,0.7) 0%, rgba(0,0,0,0.3) 70%, rgba(0,0,0,0) 100%)',
  // },
  // icon: {
  //   color: 'rgba(255, 255, 255, 0.54)',
  // },
}));

interface video_meta {
  img: string;
  src: string;
  key: string;
  model: string;
  framerate: number;
  width: number;
  height: number;
}

interface json_data {
  img: string;
  labels: any;
}

interface json_response {
  key: string;
  data: json_data;
}

interface model_meta {
  model: string;
  parameters: object;
  labels: string[] | null;
}

let modelParameterMap: {[key: string]: model_meta} = {
  "resnet": {
    "model": "Resnet50Stream",
    "parameters": {
      "share": false
    },
    "labels": resnet_labels,
  },
  "yolo": {
    "model": "AksDetectStream",
    "parameters": {
      "share": false,
      "aks_graph_name": "yolov3_cityscapes",
      "aks_graph": "${AKS_ROOT}/graph_zoo/graph_yolov3_cityscapes_256_512_u200_u250.json"
    },
    "labels": cityscapes_labels,
  },
  "facedetect": {
    "model": "AksDetectStream",
    "parameters": {
      "share": false,
      "aks_graph_name": "facedetect",
      "aks_graph": "${AKS_ROOT}/graph_zoo/graph_facedetect_u200_u250_amdinfer.json"
    },
    "labels": null,
  }
}

function InvertVideoDemo() {
  const empty_obj_array: object[] = Array(12).fill(null);
  const empty_num_array: number[] = Array(12).fill(0);
  const classes = useStyles();

  const [isLoading, setLoading] = useState(false);
  const [response, setResponse] = useState(empty_obj_array);
  const [frame_counts, setFrameCounts] = useState(empty_num_array);

  let animation_id = 0;
  const ws = useRef<WebSocket|undefined>(undefined);
  let frames: {[key: string]: Queue} = {};
  let videos = [
    {src: "/workspace/amdinfer/tests/assets/adas.webm", model: "yolo", img: "", key: "0", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/c/c4/Hanoi_traffic.ogv", model: "yolo", img: "", key: "1", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/f/f4/Traffic_control_in_Mexico_City.ogv", model: "yolo", img: "", key: "2", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/a/a7/Driving_through_the_Continuous-Flow_Intersection_Utah_SR-154_Bangerter_Hwy_at_4100_South_2013-07.ogv", model: "yolo", img: "", key: "3", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/2/23/Koala.ogv", model: "resnet", img: "", key: "4", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/7/7a/Cub_polar_bear_is_nursing_2.ogv", model: "resnet", img: "", key: "5", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/1/11/Phreatic_eruption_of_Taal_Volcano%2C_12_January_2020.webm", model: "resnet", img: "", key: "6", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/c/cf/Gorilla_gorilla_gorilla4.ogv", model: "resnet", img: "", key: "7", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/d/dc/Bilboko_Karmele_Kortajarena.webm", model: "facedetect", img: "", key: "8", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/7/7f/Zach_Britton_on_his_blown_save_to_the_Chicago_White_Sox.webm", model: "facedetect", img: "", key: "9", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/b/b5/Bill_Nye_Thanks_Teachers.ogv", model: "facedetect", img: "", key: "10", framerate: 30, width: 640, height: 360},
    {src: "https://upload.wikimedia.org/wikipedia/commons/c/c4/Physicsworks.ogv", model: "facedetect", img: "", key: "11", framerate: 30, width: 640, height: 360},
  ]
  const requested_frame_num = 200;

  let then = Array(videos.length);
  const loadMutation = useMutation(postLoad)

  function startAnimation() {
    then.fill(window.performance.now());
    animate();
  }

  function animate() {
    // calc elapsed time since last loop
    let now = window.performance.now();
    let elapsed = then.map((old_time: number) => {
      return now - old_time;
    });

    // if enough time has elapsed, draw the next frame
    let update = false;
    let update_imgs = videos.map((video: video_meta, index) => {
      // let new_img = videos[index].img;
      let resp = null;
      if (elapsed[index] > (1000 / video.framerate)) {
        // Get ready for next frame by setting then=now, but...
        // Also, adjust for (1000 / framerate) not being multiple of 16.67
        then[index] = now - (elapsed[index] % (1000 / video.framerate));

        if(!frames[video.key].isEmpty()){
          resp = frames[video.key].dequeue();
          frame_counts[index] = (frame_counts[index] + 1) % requested_frame_num;
          if (frame_counts[index] === Math.trunc(requested_frame_num * 0.5)){
            submitRequest(videos[index]);
          }
          setFrameCounts(frame_counts);
          videos[index].img = resp.img;
          update = true;
          resp["width"] = video.width;
          resp["height"] = video.height;
        }
      }
      return resp;
    });
    if(update){
      setResponse(update_imgs);
    }

    // request another frame
    animation_id = requestAnimationFrame(animate);
  }

  function connect(url: string) {
    return new Promise<WebSocket>(function(resolve, reject) {

      let server = new WebSocket(url);
      ws.current = server;
      server.onopen = function() {
        resolve(server);
      };
      server.onerror = function(err) {
        reject(err);
      };
      server.onclose = (event: any) => {
        console.log("websocket closed");
      };
      server.onmessage = (message: any) => {
        try{
          let message_obj: json_response = JSON.parse(message["data"]);
          if(message_obj.data.img.startsWith("data:")){
            frames[message_obj.key].enqueue(message_obj.data);
          } else {
            let index = parseInt(message_obj.key, 10);
            let framerate = parseFloat(message_obj.data.img);
            videos[index].framerate = Math.min(framerate, 30);
            let width = parseFloat(message_obj.data.labels[0]);
            videos[index].width = width;
            let height = parseFloat(message_obj.data.labels[1]);
            videos[index].height = height;
          }
        } catch(error){
          console.log(error);
          console.log(message["data"]);
        }
      };
    });
  }

  const submitRequest = (video: video_meta) => {
    if(!(video.key in frames)){
      frames[video.key] = new Queue();
    }
    let message = JSON.stringify({
      "model": localStorage.getItem(video.model),
      "inputs": [
        {
          "name": "input",
          "shape": [],
          "datatype": "STRING",
          "data": [video.src],
          "parameters": {
            "count": requested_frame_num
          }
        }
      ],
      "parameters": {
        "key": video.key
      }
    });
    if (ws.current !== undefined){
      ws.current.send(message);
    } else {
      console.log("Websocket connection broken!");
    }

  }

  const handleSubmit = async (event: any) => {
    const form = event.currentTarget;
    if(form.formData.value !== ""){
      const urls = form.formData.value.split(",\n");
      for(let i = 0; i < urls.length; i++){
        videos[i].src = urls[i];
      }
    }
    try{
      await connect(getWsURL() + "/models/infer");
      setResponse(empty_obj_array);
      setFrameCounts(empty_num_array);
      videos.forEach(submitRequest);
      setLoading(false);
      startAnimation();
    } catch (error){
      console.log(error);
      setLoading(false);
    }
  }

  const handleClick = () => setLoading(true);

  const handleClose = () => {
    if(ws.current != null){
      ws.current.close();
    }
    if(animation_id !== 0){
      cancelAnimationFrame(animation_id);
    }
  }

  const handleLoad = async (event: any) => {
    setLoading(true);
    try{
      await Promise.all(videos.map( async (video: video_meta) => {
        const response = await loadMutation.mutateAsync({
          model: modelParameterMap[video.model]["model"],
          parameters: modelParameterMap[video.model]["parameters"]
        })
        const body = await response.text();
        console.log(modelParameterMap[video.model]["model"]);
        if (body !== null){
          modelParameterMap[video.model]["model"] = body;
          localStorage.setItem(video.model, body);
        }
      }));
    } catch(error){
      console.log(error);
    }
    setLoading(false);
  };

  return (
    <div>
      <Form onSubmit={handleSubmit}>
        <Form.Group controlId="formData">
          <Form.Label>Input Data</Form.Label>
          <Form.Control as="textarea" rows={1} placeholder="Comma-separated paths. Leave empty to use default" />
          <Form.Text className="text-muted">
            Note: there's no data validation yet!
          </Form.Text>
        </Form.Group>

        <Button
          variant="primary"
          disabled={isLoading}
          type="submit"
          onClick={!isLoading ? handleClick : undefined}
        >
          {isLoading ? "Loading…" : "Submit Videos"}
        </Button>
        <Button
          variant="secondary"
          disabled={isLoading}
          type="button"
          onClick={!isLoading ? handleLoad : undefined}
        >
          {isLoading ? "Loading…" : "Load Models"}
        </Button>
        <Button
          variant="secondary"
          type="button"
          onClick={handleClose}
        >
          {"Close Connection"}
        </Button>
      </Form>
      <br/>
      <div className={classes.root}>
        <GridList cellHeight={360} cols={4} spacing={4}>
          {videos.map((video, index) => (
            <GridListTile key={video.key}>
              {/* {Frame(tile.img)} */}
              {/* <img src={response[index]} height={360} width={640}/> */}
              <StreamingCanvas imageStr={response[index]} labels={modelParameterMap[videos[index]["model"]]["labels"]} height={360} width={640}/>
            </GridListTile>
          ))}
        </GridList>
      </div>
    </div>
  );

}

function StreamingDemo() {
  return (
    InvertVideoDemo()
  )
}

export default StreamingDemo;
