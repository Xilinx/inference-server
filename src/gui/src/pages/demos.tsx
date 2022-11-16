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

import Accordion from "react-bootstrap/Accordion"
import Card from "react-bootstrap/Card"
import CardColumns from "react-bootstrap/CardColumns"
import Button from "react-bootstrap/Button"
import Form from "react-bootstrap/Form"
import {useMutation} from "react-query"
import {useState} from "react"
import Jimp from "jimp"

import {getWsURL, postEcho, postInvertImage, postLoad} from "../api/predict_api_v2"
import {Canvas} from "../components/Canvas"
import Queue from "../helpers/Queue"

function getData(data: any) {
  return data.data[0];
}

function EchoDemo() {
  const [isLoading, setLoading] = useState(false);
  const [response, setResponse] = useState("No response yet...");

  const mutation = useMutation(postEcho);
  const loadMutation = useMutation(postLoad)

  const handleSubmit = (event: any) => {
    const form = event.currentTarget;
    setResponse("No response yet...")
    if (isLoading) {
      mutation.mutateAsync(form.formData.value).then(data => {
        setLoading(false);
        setResponse(data.outputs.map(getData).join(","));
      })
      .catch(err => {
        console.log(err);
        setResponse("Error!");
        setLoading(false);
      });
    }

  };

  const handleLoad = (event: any) => {
    setLoading(true);
    loadMutation.mutateAsync({model: "echo", parameters: {"share": false}}).then(status => {
      if(!status){
        console.log("Returned non-OK response!");
        setResponse("Error!");
      }
      setLoading(false);
    })
    .catch(err => {
      console.log(err);
      setResponse("Error!");
      setLoading(false);
    });

  };

  const handleClick = () => setLoading(true);

  return (
    <div>
      <Form onSubmit={handleSubmit}>
        <Form.Group controlId="formData">
          <Form.Label>Input Data</Form.Label>
          <Form.Control as="textarea" rows={1} placeholder="1, 2, 3..." />
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
          {isLoading ? "Loading…" : "Submit"}
        </Button>
        <Button
          variant="secondary"
          disabled={isLoading}
          type="button"
          onClick={!isLoading ? handleLoad : undefined}
        >
          {isLoading ? "Loading…" : "Load"}
        </Button>
      </Form>
      <h4>
        <br/> Response is: {response}
      </h4>
    </div>
  );

}

function InvertImageDemo() {
  const blankImage = new ImageData(1, 1);

  const [isLoading, setLoading] = useState(false);
  const [response, setResponse] = useState(blankImage);
  const [response_b64, setResponseB64] = useState("");

  const mutation = useMutation(postInvertImage);
  const loadMutation = useMutation(postLoad);

  const handleSubmit = async (event: any) => {
    const t0 = performance.now();
    const form = event.target;
    let reader = new FileReader();
    setResponse(blankImage);
    setResponseB64("");
    if (isLoading) {
      let file = form[0].files[0];
      // reader.readAsArrayBuffer(file);
      reader.readAsDataURL(file);
    }
    // reader.onload = async function() {
    //   let arrayBuffer = this.result;
    //   if (arrayBuffer instanceof ArrayBuffer){
    //     let array = Buffer.from(arrayBuffer);
    //     let image;
    //     try{
    //       image = await Jimp.read(array)
    //     }
    //     catch(error){
    //       console.log(error);
    //       return;
    //     }

    //     const width = image.getWidth();
    //     const height = image.getHeight();

    //     try {
    //       const data = await mutation.mutateAsync({
    //         data: image.bitmap.data,
    //         height: height,
    //         width: width
    //       })
    //       let response_data = data.outputs[0].data;
    //       let resp_array = Uint8ClampedArray.from(response_data);
    //       let imageData = new ImageData(resp_array, width);
    //       setResponse(imageData);
    //       setLoading(false);
    //       var t1 = performance.now()
    //       console.log("Call to doSomething took " + (t1 - t0) + " milliseconds.")
    //     } catch (error) {
    //       console.log(error);
    //       setResponse(blankImage);
    //       setLoading(false);
    //     }
    //   }
    // }
    reader.onload = async function() {
      let imgBase64 = reader.result as string;
      if (imgBase64 != null){
        let split_image = imgBase64.split(",");
        let header = split_image[0];
        let img = split_image[1];;
        try {
          const data = await mutation.mutateAsync({
            // the product of the height and width is used to size the decoded
            // string in AMD Inference Server
            data: [img],
            height: img.length,
            width: 1
          })
          let response_data = data.outputs[0].data[0];
          setResponseB64(header + "," + response_data);
          setLoading(false);
          var t1 = performance.now()
          console.log("Call to InvertImage took " + (t1 - t0) + " milliseconds.")
        } catch (error) {
          console.log(error);
          setResponse(blankImage);
          setLoading(false);
        }
      }
    }
  }

  const handleLoad = (event: any) => {
    setLoading(true);
    loadMutation.mutateAsync({model: "InvertImage", parameters: {"share": false}}).then(status => {
      if(!status){
        console.log("Returned non-OK response!");
        setResponse(blankImage);
      }
      setLoading(false);
    })
    .catch(err => {
      console.log(err);
      setResponse(blankImage);
      setLoading(false);
    });

  };

  const handleClick = () => setLoading(true);

  return (
    <div>
      <Form onSubmit={handleSubmit}>
        <Form.Group controlId="formData">
          <Form.Label>Input Data</Form.Label>
          <Form.File id="uploadFile" label="Upload image" />
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
          {isLoading ? "Loading…" : "Submit"}
        </Button>
        <Button
          variant="secondary"
          disabled={isLoading}
          type="button"
          onClick={!isLoading ? handleLoad : undefined}
        >
          {isLoading ? "Loading…" : "Load"}
        </Button>
      </Form>
      <br/>
      <Canvas imageData={response} imageStr={response_b64} height={224} width={224}/>
    </div>
  );

}

function InvertVideoDemo() {
  const [isLoading, setLoading] = useState(false);
  const [response, setResponse] = useState("");

  let framerate = 30.0;
  let frames = new Queue();
  const loadMutation = useMutation(postLoad);

  let then:number;

  function startAnimation() {
    then = window.performance.now();
    animate();
  }

  function animate() {
    // request another frame
    requestAnimationFrame(animate);

    // calc elapsed time since last loop
    let now = window.performance.now();;
    let elapsed = now - then;

    // if enough time has elapsed, draw the next frame
    if (elapsed > (1000 / framerate)) {
      // Get ready for next frame by setting then=now, but...
      // Also, adjust for (1000 / framerate) not being multiple of 16.67
      then = now - (elapsed % (1000 / framerate));

      if(!frames.isEmpty()){
        setResponse(frames.dequeue());
      }
    }
  }

  // function updateFrame(){
  //   setTimeout(function() {
  //     if(!frames.isEmpty()){
  //       setResponse(frames.dequeue());
  //     }
  //     window.requestAnimationFrame(updateFrame);
  //   }, 1000 / framerate);
  // }

  function connect(url: string) {
    return new Promise<WebSocket>(function(resolve, reject) {
      var server = new WebSocket(url);
      server.onopen = function() {
        resolve(server);
      };
      server.onerror = function(err) {
        reject(err);
      };
      server.onclose = (event: any) => {
        console.log("closed");
      };
      server.onmessage = (message: any) => {
        let message_obj = JSON.parse(message["data"]);
        if(message_obj.data.startsWith("data:")){
          frames.enqueue(message_obj.data);
        } else {
          framerate = parseFloat(message_obj.data);
        }
      };
    });
  }

  const handleSubmit = async (event: any) => {
    const form = event.currentTarget;
    try{
      let socket = await connect(getWsURL() + "/models/infer");
      let message = JSON.stringify({
        "model": "InvertVideo",
        "inputs": [
          {
            "name": "input",
            "shape": [],
            "datatype": "STRING",
            "data": [form.formData.value]
          }
        ],
        "parameters": {
          "key": "0",
          "share": false
        }
      });
      socket.send(message);
      setLoading(false);
      startAnimation();
    } catch (error){
      console.log(error);
      setLoading(false);
    }
  }

  const handleLoad = (event: any) => {
    setLoading(true);
    loadMutation.mutateAsync({model: "InvertVideo", parameters: {"share": false}}).then(status => {
      if(!status){
        console.log("Returned non-OK response!");
      }
      setLoading(false);
    })
    .catch(err => {
      console.log(err);
      setLoading(false);
    });

  };

  const handleClick = () => setLoading(true);

  return (
    <div>
      <Form onSubmit={handleSubmit}>
        <Form.Group controlId="formData">
          <Form.Label>Input Data</Form.Label>
          <Form.Control as="textarea" rows={1} placeholder="Path to file..." />
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
          {isLoading ? "Loading…" : "Submit"}
        </Button>
        <Button
          variant="secondary"
          disabled={isLoading}
          type="button"
          onClick={!isLoading ? handleLoad : undefined}
        >
          {isLoading ? "Loading…" : "Load"}
        </Button>
      </Form>
      <br/>
      <img src={response} height={360} width={640}/>
    </div>
  );

}

function Demos() {
  return (
    <div>
    <CardColumns>
      <Card className="p-3" border="light" style={{ width: '22rem' }}>
        <Card.Title> Echo </Card.Title>
        <Card.Text as="div">
          The Echo worker accepts integers, adds one to each and returns these sums.
          <br/>
          <Accordion>
            <Card style={{ width: '20rem' }}>
              <Card.Header>
                <Accordion.Toggle as={Button} variant="link" eventKey="0">
                  Try it!
                </Accordion.Toggle>
              </Card.Header>
              <Accordion.Collapse eventKey="0">
                <Card.Body>{EchoDemo()}</Card.Body>
              </Accordion.Collapse>
            </Card>
          </Accordion>
        </Card.Text>
      </Card>
      <Card className="p-3" border="light" style={{ width: '22rem' }}>
        <Card.Title> InvertImage </Card.Title>
        <Card.Text as="div">
          The InvertImage worker accepts an image and returns it with its colors
          inverted.
          <br/>
          <Accordion>
            <Card style={{ width: '20rem' }}>
              <Card.Header>
                <Accordion.Toggle as={Button} variant="link" eventKey="0">
                  Try it!
                </Accordion.Toggle>
              </Card.Header>
              <Accordion.Collapse eventKey="0">
                <Card.Body>{InvertImageDemo()}</Card.Body>
              </Accordion.Collapse>
            </Card>
          </Accordion>
        </Card.Text>
      </Card>
    </CardColumns>
    <Card className="p-3" border="light" style={{ width: '700px' }}>
      <Card.Title> InvertVideo </Card.Title>
      <Card.Text as="div">
        The InvertVideo worker accepts a path (relative to the server) to an video file.
        <br/>
        <Accordion>
          <Card style={{ width: '685px' }}>
            <Card.Header>
              <Accordion.Toggle as={Button} variant="link" eventKey="0">
                Try it!
              </Accordion.Toggle>
            </Card.Header>
            <Accordion.Collapse eventKey="0">
              <Card.Body>{InvertVideoDemo()}</Card.Body>
            </Accordion.Collapse>
          </Card>
        </Accordion>
      </Card.Text>
    </Card>
  </div>
  )
}

export default Demos;
