/**
 * Copyright 2021 Xilinx Inc.
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

import pako from "pako"

import {kDefaultHttpPort} from '../build_options'
import fetchTimeout from "../helpers/fetchTimeout"

function getUrl(){
  let url = "";
  if(process.env.NODE_ENV === "development") {
    url += "127.0.0.1:" + kDefaultHttpPort;
  }

  if(process.env.NODE_ENV === "production") {
    url += window.location.host;
  }
  return url;
}

function getHttpURL(){
  let url = "http://";
  url += getUrl();
  return url;
}

function getWsURL(){
  let url = "ws://";
  url += getUrl();
  return url;
}

async function sendGetRequest(endpoint: string){
  const response = await fetchTimeout(getHttpURL() + endpoint, 20000);
  if (!response.ok) {
    throw new Error(response.statusText)
  }
  return response.json()
}

async function sendPostRequest(endpoint: string, body: any){
  const response = await fetchTimeout(getHttpURL() + endpoint, 5000, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Content-Encoding': 'deflate'
    },
    body: pako.deflate(body)
    // body: body
  });
  if (!response.ok) {
    throw new Error(response.statusText)
  }
  return response.json()
}

async function sendSimplePostRequest(endpoint: string, body: any){
  const response = await fetchTimeout(getHttpURL() + endpoint, 20000, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Content-Encoding': 'deflate'
    },
    body: body
  });
  if (!response.ok) {
    throw new Error(response.statusText)
  }
  return response
}

async function getServerMetadata() { return sendGetRequest("/v2") }

async function postEcho(data: any) {
  let body = {
    "inputs": data.split(",").map(function(data: string) {
      return (
        {
          "name": "input",
          "shape": [1],
          "datatype": "UINT32",
          "data": [Number(data)]
        }
      )
    })
  }
  return sendPostRequest("/v2/models/echo/infer", JSON.stringify(body));
}

interface loadRequest {
  model: string;
  parameters: object;
}

async function postLoad({model, parameters}: loadRequest) {

  let body:any = {}
  body.model_name = model
  if(parameters !== null){
    body.parameters = parameters
  }
  let endpoint:string = "v2/repository/models/" + model + "/load"
  return sendSimplePostRequest(endpoint, JSON.stringify(body));
}

interface invertImageRequest {
  data: Uint8Array | Array<string>;
  height: number;
  width: number;
}

async function postInvertImage({data, height, width}: invertImageRequest) {
  let my_body;
  if (data instanceof Uint8Array){
    let body = {
      "inputs": [
        {
          "name": "input",
          "shape": [height, width, 4],
          "datatype": "UINT8",
          "data": data
        }
      ]
    }
    /*
    * This weird hack is needed because calling stringify on the Uint8Array
    * directly turns it into:
    * {
    *   "type": "Buffer"
    *   "data": [...]
    * }
    * And we just need the data part.
    */
    my_body = JSON.stringify(body, (key, value) => {
      if(key === "data"){
        return value["data"];
      }
      return value;
    });
  } else {
    let body = {
      "inputs": [
        {
          "name": "input",
          "shape": [height, width, 4],
          "datatype": "STRING",
          "data": data
        }
      ]
    }
    my_body = JSON.stringify(body);
  }

  return sendPostRequest("/v2/models/InvertImage/infer", my_body);
}

export {
  getServerMetadata,
  postEcho,
  postInvertImage,
  getWsURL,
  postLoad
};
