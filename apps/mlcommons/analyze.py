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

import numpy as np
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go


def graph_qps(df: pd.DataFrame, model, scenario, protocol):

    if scenario == "SingleStream":
        qps_column = "qps"
    elif scenario == "Server":
        qps_column = "completed_samples_per_sec"
    else:
        return

    fig = go.Figure()
    fig.add_trace(
        go.Bar(
            name=protocol,
            x=df["protocol"],
            y=df[qps_column],
        )
    )
    fig.update_layout(
        xaxis={"title": "Protocol"},
        yaxis_title="Queries per Second",
        title_text=f"{scenario} ({model})",
    )
    fig.write_image(f"{model}_{scenario}_protocols_qps.jpg")
    fig.write_json(f"{model}_{scenario}_protocols_qps.json")


def graph_protocols(df: pd.DataFrame):
    models = df["model"].unique()
    scenarios = df["scenario"].unique()

    for model, scenario in itertools.product(models, scenarios):
        filtered_df = df[
            (df["model"] == model) & (df["scenario"] == scenario)
        ].sort_values("protocol")

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
            xaxis={"title": "Time (s)", "showexponent": "all", "exponentformat": "B"},
            yaxis_title="Percentile",
            legend_title_text="Protocol",
            title_text=f"{scenario} ({model})",
        )
        fig.write_image(f"{model}_{scenario}_protocols.jpg")
        fig.write_json(f"{model}_{scenario}_protocols.json")

        graph_qps(filtered_df, model, scenario, protocol)


def main(args: argparse.Namespace):
    with open(args.data, "rb") as f:
        logs = pickle.load(f)

    if args.delete:
        logs.df = logs.df.drop(args.delete)
        with open(args.data, "wb") as f:
            pickle.dump(logs, f)

    df: pd.DataFrame = logs.df

    if args.print:
        print(df)

    graph_protocols(df)


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
    parser.add_argument(
        "--delete",
        nargs="+",
        type=int,
        help="Delete these rows by index. THIS IS DESTRUCTIVE",
    )
    args = parser.parse_args()

    main(args)
