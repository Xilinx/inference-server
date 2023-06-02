# Copyright 2023 Advanced Micro Devices, Inc.
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

import argparse
import itertools
import pickle

import common
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go


def graph_protocols(logs: common.MlcommonsLogs):
    df: pd.DataFrame = logs.df
    models = df["model"].unique()
    scenarios = df["scenario"].unique()

    for model, scenario in itertools.product(models, scenarios):
        filtered_df = df[(df["model"] == model) & (df["scenario"] == scenario)]

        protocols = filtered_df["protocol"].unique()

        graph_data = (
            filtered_df[
                [
                    "protocol",
                    "min_latency",
                    "latency_50_0",
                    "latency_90_0",
                    "latency_95_0",
                    "latency_97_0",
                    "latency_99_0",
                    "latency_99_9",
                ]
            ]
            .set_index("protocol")
            .transpose()
        )
        graph_data["percentile"] = [0, 50, 90, 95, 97, 99, 99]

        # fig = px.line(bar, x=protocols, y="percentile", markers=True, symbol="protocol")

        symbols = ["circle", "square", "diamond"]
        fig = go.Figure()
        for index, protocol in enumerate(protocols):
            fig.add_trace(
                go.Scatter(
                    name=protocol,
                    mode="markers+lines",
                    x=graph_data[protocol],
                    y=graph_data["percentile"],
                    marker={"symbol": symbols[index]},
                )
            )

        fig.update_layout(
            xaxis_title="Time (ns)",
            yaxis_title="Percentile",
            legend_title_text="Protocol",
            title_text=f"{scenario} ({model})",
        )
        fig.write_image(f"{model}_{scenario}_protocols.jpg")
        fig.write_json(f"{model}_{scenario}_protocols.json")


def main(args: argparse.Namespace):
    with open(args.data, "rb") as f:
        logs = pickle.load(f)

    if args.print:
        print(logs)

    graph_protocols(logs)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Run mlcommons benchmarks")
    parser.add_argument(
        "--data",
        default="mlcommons.bin",
        help="Path to file to analyze. Defaults to mlcommons.bin",
    )
    parser.add_argument(
        "--print",
        action="store_true",
        help="Print the raw data",
    )
    args = parser.parse_args()

    main(args)
