#!/usr/bin/env python3

# This script is used to join and clean the calibration data for use in the 'cost_models' script
# It takes the directory of the four .csv files that are generated by the cost calibration run on hyrise as an argument

import json
import numpy as np
import os
import pandas as pd


def parse_csv_meta(file_name):
    with open(file_name) as f:
        csv_meta = json.loads(f.read())
        headers = [column["name"] for column in csv_meta["columns"]]
        config = csv_meta["config"]
        return (headers, config["separator"])


def parse_hyrise_csv(file_name):
    headers, separator = parse_csv_meta(f"{file_name}.json")
    return pd.read_csv(file_name, sep=separator, names=headers)


def import_joined_data(path):
    if not path.endswith(os.sep):
        path += os.sep
    operator_data = parse_hyrise_csv(path + "operators.csv")
    table_data = parse_hyrise_csv(path + "table_meta.csv")
    columns_data = parse_hyrise_csv(path + "column_meta.csv")
    chunk_meta = parse_hyrise_csv(path + "segment_meta.csv")

    # remove some outlier
    sdev = np.std(operator_data["RUNTIME_NS"])
    mean = np.mean(operator_data["RUNTIME_NS"])
    operator_data = operator_data[operator_data["RUNTIME_NS"] < mean + 3 * sdev]

    joined_data = operator_data.merge(table_data, on=["TABLE_NAME"], how="left")
    joined_data = joined_data.merge(columns_data, on=["TABLE_NAME", "COLUMN_NAME"], how="left")

    # only look at the encoding of the first chunk
    chunkrows = chunk_meta.loc[(chunk_meta["CHUNK_ID"] == 0)]
    joined_data = joined_data.merge(chunkrows, on=["TABLE_NAME", "COLUMN_NAME"], how="left")

    joined_data = joined_data.rename(
        columns={"CHUNK_SIZE": "MAX_CHUNK_SIZE", "COLUMN_DATA_TYPE": "DATA_TYPE", "ENCODING_TYPE": "ENCODING"}
    )

    # remove the rows with ExpressionEvaluator queries from the test data since we don't have any in the test data
    joined_data = joined_data.loc[(joined_data["OPERATOR_IMPLEMENTATION"] != "ExpressionEvaluator")]

    # explicitly add selectivity
    joined_data["SELECTIVITY_LEFT"] = joined_data["OUTPUT_ROWS"] / joined_data["INPUT_ROWS_LEFT"]
    joined_data["SELECTIVITY_LEFT"].fillna(0, inplace=True)
    joined_data["SELECTIVITY_RIGHT"] = joined_data["OUTPUT_ROWS"] / joined_data["INPUT_ROWS_RIGHT"]
    joined_data["SELECTIVITY_RIGHT"].fillna(0, inplace=True)

    # remove infinite selectivities from empty inputs
    joined_data.replace(np.inf, 0, inplace=True)
    joined_data.fillna("0", inplace=True)
    return joined_data


def import_operator_data(path, filename):
    if not path.endswith(os.sep):
        path += os.sep
    joined_data = parse_hyrise_csv(path + filename)
    table_data = parse_hyrise_csv(path + "table_meta.csv")
    columns_data = parse_hyrise_csv(path + "column_meta.csv")
    chunk_meta = parse_hyrise_csv(path + "segment_meta.csv")
    chunkrows = chunk_meta.loc[(chunk_meta["CHUNK_ID"] == 0)]

    has_two_inputs = "INPUT_ROWS_LEFT" in joined_data and "INPUT_ROWS_RIGHT" in joined_data
    if has_two_inputs:
        for side in ["LEFT", "RIGHT"]:
            joined_data = joined_data.merge(table_data, left_on=f"{side}_TABLE_NAME", right_on="TABLE_NAME", how="left")
            joined_data = joined_data.merge(
                columns_data,
                left_on=[f"{side}_TABLE_NAME", f"{side}_COLUMN_NAME"],
                right_on=["TABLE_NAME", "COLUMN_NAME"],
                how="left",
            )
            joined_data = joined_data.merge(
                chunkrows,
                left_on=[f"{side}_TABLE_NAME", f"{side}_COLUMN_NAME"],
                right_on=["TABLE_NAME", "COLUMN_NAME"],
                how="left",
            )
            joined_data = joined_data.rename(
                columns={
                    "CHUNK_SIZE": f"MAX_CHUNK_SIZE_{side}",
                    "COLUMN_DATA_TYPE": f"DATA_TYPE_{side}",
                    "ENCODING_TYPE": f"ENCODING_{side}",
                    "SORTED": f"{side}_SORTED",
                    "COMPRESSION_TYPE": f"COMPRESSION_TYPE_{side}",
                }
            )
    else:
        joined_data = joined_data.merge(table_data, on=["TABLE_NAME"], how="left")
        joined_data = joined_data.merge(columns_data, on=["TABLE_NAME", "COLUMN_NAME"], how="left")
        joined_data.fillna("", inplace=True)
        # only look at the encoding of the first chunk
        joined_data = joined_data.merge(chunkrows, on=["TABLE_NAME", "COLUMN_NAME"], how="left")
        joined_data = joined_data.rename(
            columns={"CHUNK_SIZE": "MAX_CHUNK_SIZE", "COLUMN_DATA_TYPE": "DATA_TYPE", "ENCODING_TYPE": "ENCODING"}
        )
        # remove the rows with ExpressionEvaluator queries from the test data since we don't have any in the test data
        joined_data = joined_data.loc[(joined_data["OPERATOR_IMPLEMENTATION"] != "ExpressionEvaluator")]
        # joined_data = current_data.drop(labels=["TABLE_NAME", "COLUMN_NAME", "ROW_COUNT", "CHUNK_ID"], axis=1)
    drop_labels = [
        [label for label in joined_data.columns if keyword in label]
        for keyword in ["TABLE_NAME", "COLUMN_NAME", "ROW_COUNT", "CHUNK_ID"]
    ]
    joined_data = joined_data.drop(labels=np.concatenate(drop_labels), axis=1)

    # explicitly add selectivity
    input_rows_left = "INPUT_ROWS"
    if "INPUT_ROWS_LEFT" in joined_data:
        input_rows_left = "INPUT_ROWS_LEFT"
        joined_data["SELECTIVITY_RIGHT"] = joined_data["OUTPUT_ROWS"] / joined_data["INPUT_ROWS_RIGHT"]
        joined_data["SELECTIVITY_RIGHT"].fillna(0, inplace=True)
        joined_data["SELECTIVITY_RIGHT"].replace(np.inf, 0, inplace=True)
    joined_data["SELECTIVITY_LEFT"] = joined_data["OUTPUT_ROWS"] / joined_data[input_rows_left]
    joined_data["SELECTIVITY_LEFT"].fillna(0, inplace=True)
    joined_data["SELECTIVITY_LEFT"].replace(np.inf, 0, inplace=True)

    # remove infinite selectivities from empty inputs
    # joined_data.replace(np.inf, 0, inplace=True)
    # joined_data.fillna("0", inplace=True)
    # print(joined_data.columns)
    return joined_data


def import_all_operator_data(path):
    general_operator_data = import_operator_data(path, "operators.csv")
    join_data = import_operator_data(path, "joins.csv")
    join_stages_file = f"{path}{'' if path.endswith(os.sep) else os.sep}join_stages.csv"
    join_stages = parse_hyrise_csv(join_stages_file)
    join_stages = join_stages.merge(join_data, on=["JOIN_ID"], how="left")
    join_data = join_data.drop(labels=["JOIN_ID"], axis=1)
    join_stages = join_stages.drop(labels=["JOIN_ID"], axis=1)
    return general_operator_data.dropna(), join_data.dropna(), join_stages.dropna()