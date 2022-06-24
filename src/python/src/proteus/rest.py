# Copyright 2021 Xilinx Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
This module defines how to communicate to a proteus-server using the REST API.

Classes:
    Client - provides methods to communicate to a proteus-server over REST
"""

import json
import requests
import asyncio
import time
import zlib

import aiohttp

from . import exceptions
from .predict_api import (
    ErrorResponse,
    InferenceRequest,
    InferenceResponse,
    HtmlResponse,
)


class _Response:
    def __init__(self, resp, content):
        self._resp = resp
        self._content = content

    def __getattr__(self, name):
        if hasattr(self._resp, name):
            return getattr(self._resp, name)
        raise AttributeError(f"'Response' object has no attribute '{name}'")

    @property
    def status_code(self):
        return self._resp.status

    def json(self):
        return self._content

    def text(self):
        return self._content


async def _async_get(session, url):
    resp = await session.get(url)
    async with resp:
        content_type = resp.headers.get("content-type")
        if content_type is None:
            raise exceptions.BadResponseError("Content type is None")
        elif "application/json" in content_type:
            content = await resp.json()
        elif "text/html" in content_type:
            content = await resp.text()
        else:
            raise exceptions.BadResponseError(f"Unknown content type: {content_type}")
        return _Response(resp, content)


async def _async_post(session, url, body):
    resp = await session.post(url, json=body)
    async with resp:
        content_type = resp.headers.get("content-type")
        if content_type is None:
            return None
        elif "application/json" in content_type:
            content = await resp.json()
        elif "text/html" in content_type:
            content = await resp.text()
        else:
            return None
        return _Response(resp, content)


def _parse_infer_response(response: requests.Response):
    if response.status_code == 200:
        content_type = response.headers.get("content-type")
        if content_type is not None and "application/json" in content_type:
            content = response.json()
            if "error" not in content:
                return InferenceResponse(content)
    return ErrorResponse(response)


class Client:
    """
    The Client class provides methods to communicate to the proteus-server.
    """

    def __init__(self, address, headers=None):
        self.http_addr = "http://" + address
        self.https_addr = "https://" + address
        self.headers = headers if headers is not None else {}

    def _get(self, endpoint, error_str):
        url = f"{self.http_addr}/{endpoint}"
        try:
            response = requests.get(url, headers=self.headers)
        except requests.ConnectionError:
            raise exceptions.ConnectionError(
                error_str + f"Cannot connect to Proteus at {url}."
            ) from None
        except requests.exceptions.MissingSchema:
            raise exceptions.ConnectionError(
                error_str + f"Invalid URL: {url}"
            ) from None

        return response

    async def _async_get(self, endpoints: list, error_str: str):
        async with aiohttp.ClientSession() as session:
            tasks = []
            for endpoint in endpoints:
                url = f"{self.http_addr}/{endpoint}"
                tasks.append(asyncio.ensure_future(_async_get(session, url)))
            return await asyncio.gather(*tasks)

    def _gets(self, endpoints, error_str):
        loop = asyncio.get_event_loop()
        return loop.run_until_complete(self._async_get(endpoints, error_str))

    def _post(self, endpoint, body, error_str, compress=False):
        if isinstance(body, InferenceRequest):
            body = body.asdict()

        if compress:
            self.headers["content-encoding"] = "gzip"
            body = zlib.compress(json.dumps(body).encode())
            try:
                response = requests.post(
                    f"{self.http_addr}/{endpoint}", data=body, headers=self.headers
                )
            except requests.ConnectionError:
                raise exceptions.ConnectionError(
                    error_str + "Cannot connect to Proteus."
                )
        else:
            try:
                print('debug point in _post.\n  http_addr: \n')
                print(self.http_addr,"\nendpoint:\n", endpoint, '\n\nbody has keys:\n', body.keys(),'\nheaders:\n', self.headers, '\n\n')
                
                response = requests.post(
                    f"{self.http_addr}/{endpoint}", json=body, headers=self.headers
                )
            except requests.ConnectionError:
                raise exceptions.ConnectionError(
                    error_str + "Cannot connect to Proteus."
                ) from None

        return response

    async def _async_post(self, endpoints: list, bodies: list, error_str: str):
        async with aiohttp.ClientSession() as session:
            tasks = []
            for endpoint, body in zip(endpoints, bodies):
                url = f"{self.http_addr}/{endpoint}"
                if isinstance(body, InferenceRequest):
                    body = body.asdict()
                tasks.append(asyncio.ensure_future(_async_post(session, url, body)))
            return await asyncio.gather(*tasks)

    def _posts(self, endpoints, bodies, error_str):
        loop = asyncio.get_event_loop()
        return loop.run_until_complete(self._async_post(endpoints, bodies, error_str))

    def load(self, model: str, parameters=None):
        """
        Load a model

        Args:
            model (str): Name of the model to load
            parameters (dict, optional): Load-time parameters to pass to Proteus. Defaults to None.

        Returns:
            Response: Returns an HtmlResponse with the qualified name to make inference requests to
        """
        request = {"model_name": model}
        if parameters:
            request["parameters"] = parameters
        error_str = f"Failed to load {model}. "
        endpoint = self.get_endpoint("load", model)

        response = self._post(endpoint, request, error_str)

        if (
            "application/json" in response.headers.get("content-type")
            or response.status_code != 200
        ):
            return ErrorResponse(response)
        return HtmlResponse(response.content.decode("utf-8"))

    def unload(self, model):
        """
        Unload a model

        Args:
            model (str): Qualified name of the model to unload

        Returns:
            Response: HtmlResponse if success, ErrorResponse if failure
        """
        request = {"model_name": model}
        error_str = f"Failed to unload {model}. "
        endpoint = self.get_endpoint("unload", model)

        response = self._post(endpoint, request, error_str)
        content_type = response.headers.get("content-type")
        if content_type is None or "application/json" in content_type:
            return ErrorResponse(response)
        return HtmlResponse(response.content.decode("utf-8"))

    def model_ready(self, model):
        """
        Check if a particular model is ready

        Args:
            model (str): Qualified name of the model to check

        Returns:
            bool: True if ready
        """
        error_str = f"Failed to check if {model} is ready. "
        endpoint = self.get_endpoint("model_ready", model)
        response = self._get(endpoint, error_str)
        return response.status_code == 200

    def infer(self, model, request, compress=False):
        """
        Make an inference request from a model

        Args:
            model (str): Qualified name of the model to make a request to
            request (Request|dict): Request to make
            compress (bool, optional): Compress the request. Defaults to False.

        Returns:
            Response: JsonResponse if success, ErrorResponse if failure
        """
        response = self._infer(model, request, compress)
        return _parse_infer_response(response)

    def infers(self, models: list, requests):
        """
        Make multiple inference requests to multiple models. This method launches
        multiple asynchronous requests simultaneously and blocks until all return.
        The number of models and requests should match.

        Args:
            models (list): List containing qualified names of models to make inferences
            requests (list): List containing Requests or dicts

        Returns:
            list: List containing responses for each request
        """
        endpoints = [self.get_endpoint("infer", model) for model in models]
        error_str = f"Failed to infer from one of {str(models)}."

        responses = self._posts(endpoints, requests, error_str)
        return [_parse_infer_response(response) for response in responses]

    def _infer(self, model, request, compress=False):
        error_str = f"Failed to infer from {model}."
        endpoint = self.get_endpoint("infer", model)
        return self._post(endpoint, request, error_str, compress)

    def server_live(self):
        """
        Check if the server is live

        Returns:
            bool: True if live
        """
        error_str = f"Failed to check if server is live. "
        endpoint = self.get_endpoint("server_live")
        response = self._get(endpoint, error_str)
        return response.status_code == 200

    def wait_until_live(self):
        """
        Block until the server is live
        """
        status = False
        while not status:
            try:
                status = self.server_live()
            except exceptions.ConnectionError:
                time.sleep(1)

    def wait_until_stop(self):
        """
        Block until the server is dead
        """
        is_up = True
        while is_up:
            try:
                if self.server_live():
                    time.sleep(1)
            except exceptions.ConnectionError:
                is_up = False

    def get_endpoint(self, command, *args):
        """
        Get the REST endpoint corresponding to a particular command. If the command
        takes arguments, pass them as well.

        Args:
            command (str): Name of the command to get the endpoint of

        Returns:
            str: REST endpoint
        """
        arg_0 = ""
        if args:
            arg_0 = args[0]

        commands = {
            "server_live": "v2/health/live",
            "infer": f"v2/models/{arg_0}/infer",
            "model_ready": f"v2/models/{arg_0}/ready",
            "unload": f"v2/repository/models/{arg_0}/unload",
            "load": f"v2/repository/models/{arg_0}/load",
        }
        return commands[command]

    def get_address(self, command, *args):
        """
        Get the HTTP address corresponding to a particular command. If the command
        takes arguments, pass them as well.

        Args:
            command (str): Name of the command to get the endpoint of

        Returns:
            str: HTTP address
        """
        url = self.get_endpoint(command, *args)
        return f"{self.http_addr}/{url}"
