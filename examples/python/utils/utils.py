# Copyright 2022 Advanced Micro Devices, Inc.
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

import cv2
import numpy as np


def center_crop(img, dim):
    """
    Crops the input `img` from center to a specific dimention specified

    Args:
        img (numpy.ndarray): Image which needs to be cropped
        dim (tuple): Shape of the output image

    Returns:
        [numpy.ndarray]: Image croppepd to dim shape
    """

    width, height = img.shape[1], img.shape[0]

    # process crop width and height for max available dimension
    crop_width = dim[0] if dim[0] < img.shape[1] else img.shape[1]
    crop_height = dim[1] if dim[1] < img.shape[0] else img.shape[0]
    mid_x, mid_y = int(width / 2), int(height / 2)
    cw2, ch2 = int(crop_width / 2), int(crop_height / 2)
    crop_img = img[mid_y - ch2 : mid_y + ch2, mid_x - cw2 : mid_x + cw2]
    return crop_img


def _mean_image_subtraction(image, means):
    """
    Subtract mean value on each channel

    Args:
        image (numpy.ndarray): Image on which mean value have to be subtracted
        means (list): List of values matching number of channels in the image

    Raises:
        ValueError: If the image does not have 3 channels
        ValueError: If the length of means and channels does not match

    Returns:
        [numpy.ndarray]: The numpy array after subtracting mean value
    """
    if image.ndim != 3:
        raise ValueError("Input must be of size [height, width, C>0]")
    num_channels = image.shape[-1]
    if len(means) != num_channels:
        raise ValueError("len(means) must match the number of channels")

    # channels = tf.split(axis=2, num_or_size_splits=num_channels, value=image)
    channels = np.split(image, axis=2, indices_or_sections=num_channels)
    for i in range(num_channels):
        channels[i] -= means[i]
    return np.concatenate(channels, axis=2)


def preprocess(image_location, input_size, resize_method):
    """
    This image will load and preprocess the data according to
    the input arguments

    Args:
        image_location (str): The location of the image to be loaded
        input_size (int): The input height/width of the image
        resize_method (str): Which prepcessing method to use

    Returns:
        [numpy.ndarray]: The preprocessed image as a numpy array
    """

    image = cv2.imread(image_location)
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    shape = image.shape

    if resize_method == "crop" or resize_method == "xilinx":
        if shape[0] < shape[1]:
            scale = input_size / shape[0]
        else:
            scale = input_size / shape[1]
        new_shape = tuple(map(int, (shape[1] * scale, shape[0] * scale)))

        image = cv2.resize(image, new_shape)
        image = center_crop(image, (input_size, input_size)).astype(np.float32)

        if resize_method == "xilinx":
            image = _mean_image_subtraction(image, [123.68, 116.78, 103.94])

    else:  # bilinear

        if image.dtype != np.float32:
            image = (image / 255).astype(np.float32)

        new_shape = tuple(map(int, (shape[1] * 0.875, shape[0] * 0.875)))
        image = center_crop(image, new_shape)
        image = cv2.resize(image, (input_size, input_size), cv2.INTER_LINEAR)
        image = np.subtract(image, 0.5)
        image = np.multiply(image, 2.0)

    return image


def postprocess(response, k=5):
    """
    This method will postprocess the output response
    from the server and returns topK

    Args:
        response (proteus.predict_api.InferenceResponse):
            The response from the server
        k (int, optional): K value for topK.
            Defaults to 5.

    Returns:
        [numpy.ndarray]: topK values
    """
    response_data = response.outputs[0].data
    response_data = np.argsort(response_data)
    return response_data[-k:][::-1]
