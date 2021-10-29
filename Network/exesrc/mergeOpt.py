#!/usr/bin/env python3

import os
import pandas as pd
import argparse

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--input-files", nargs='+')
    parser.add_argument("--output", type=str)

    return parser.parse_args()

def main():
    args = parser()

    inputDF = []

    for f in args.input_files:
        try:
           inputDF.append(pd.read_csv(f, index_col=None, header=0, sep="\t"))

        except:
            pass

    outDF = pd.concat(inputDF, axis=0, ignore_index=True)
    outDF = outDF.sort_values(by="loss", ascending=True)

    outDF.to_csv(args.output, index=False, sep="\t")

if __name__ == "__main__":
    main()
