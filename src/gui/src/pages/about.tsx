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

import Card from "react-bootstrap/Card"
import CardColumns from "react-bootstrap/CardColumns"
import Spinner from "react-bootstrap/Spinner"
import {useQuery} from "react-query"

import {getServerMetadata} from "../api/predict_api_v2"
import {MapToUnorderedList} from "../components/MapToList"

interface Metadata {
  name: string;
  version: string;
  extensions: Array<string>;
}

// Card.Text is marked as="div" because it defaults to <p> which should only
// have text underneath. The Spinner violates this so we change it to div
function ServerMetadataCard(text: JSX.Element | string){
  return (
    <Card style={{ width: '18rem' }}>
      <Card.Body>
        <Card.Title>Server Metadata</Card.Title>
        <Card.Text as="div">
          {text}
        </Card.Text>
      </Card.Body>
    </Card>
  )
}

function RenderServerMetadata() {
  const { isLoading, isError, data, error } = useQuery<Metadata, Error>(
    "getServerMetadata", getServerMetadata, {"retry": false}
  )

  if(isLoading){
    return ServerMetadataCard(<Spinner animation="border"/>)
  }

  if(isError){
    if (error === null){
      console.log("Unknown error in fetching server metadata");
    } else {
      console.log(error.message);
    }
    return ServerMetadataCard("Error getting metadata from AMD Inference Server!")
  }

  if (data){
    let extensions;
    if(data.extensions){
      extensions = MapToUnorderedList(data.extensions)
    } else {
      extensions = "none";
    }
    return ServerMetadataCard(
      <div>
        Name: {data.name} <br/>
        Version: {data.version} <br/>
        Extensions: {extensions}
      </div>
    );
  } else {
    console.log("No data found when fetching server metadata");
    return ServerMetadataCard("No data found getting metadata from AMD Inference Server!");
  }
}

const About = () => {

  return (
    <CardColumns>
      <RenderServerMetadata/>
    </CardColumns>
  );

}

export default About;
