#!/usr/bin/env python3

from bayes_opt import BayesianOptimization, SequentialDomainReductionTransformer

import subprocess
import argparse
import os
import yaml
import pandas as pd
import numpy as np
import time
import sys
from multiprocessing import Pool, cpu_count
from functools import partial

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--config", type=str, action="store", help="Yaml config with DNN configuration")
    parser.add_argument("--type", type=str, action="store", help="Which DNN to optimize")
    return parser.parse_args()

def dnn(config, channel, batchSize, lr, nNodes, nLayers, dropOut):
    try:
        dnnDir = "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].format("Odd"), channel)

        hyperParam = ["--batch-size", str(batchSize), "--lr", str(10**(int(lr))), "--n-nodes", str(nNodes), "--n-layers", str(nLayers), "--drop-out", str(dropOut)]
        files = ["--sig-files", *["{d}/{sig}/{sig}.csv".format(d = dnnDir, sig = config["signal"].format(m)) for m in config["masses"]], "--bkg-files", *["{d}/{bkg}/{bkg}.csv".format(d = dnnDir, bkg = bkg) for bkg in config["backgrounds"]]]

        out = subprocess.run(["dnn", "--optimize", "--out-path", dnnDir] + hyperParam + files, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        return float(out.stdout.decode('utf-8').split("\n")[-2].split(" ")[-1])

    except:
        return 0.

def htag(batchSize, nHidden, nConvFilter, kernelSize, dropOut, lr):
    try:
        hyperParam = ["--batch-size", str(batchSize), "--lr", str(10**(int(lr))), "--n-hidden", str(nHidden), "--n-convfilter", str(nConvFilter), "--n-kernelsize", str(kernelSize), "--drop-out", str(dropOut)]

        out = subprocess.run(["htag", "--optimize"] + hyperParam, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        return float(out.stdout.decode('utf-8').split("\n")[-2].split(" ")[-1])

    except:
        return 0.

def optimize(func, hyperParam, hyperDir):
    os.makedirs(hyperDir, exist_ok = True)

    np.random.seed(sum(time.localtime()))
    pool = Pool(8)#cpu_count())

    param = [{key: value[0](*value[1]) for key, value in hyperParam.items()} for i in range(cpu_count())]
    jobs = [pool.apply_async(func, kwds=p) for p in param]
    nJobs = len(jobs)
    bestLoss = 1e7

    while(nJobs < 200 or len(jobs) != 0):
        for i, p in enumerate(param):
            if jobs[i].ready():
                loss = jobs[i].get()

                if(loss < bestLoss and loss != 0):
                    bestLoss = loss

                    print("Best loss: {}, Hyperparam:".format(loss), p, "Iteration:", nJobs) 
                    p["lr"] =  10**(p["lr"])

                    frame = pd.DataFrame.from_dict({k: [v] for (k, v) in p.items()})

                    frame.to_csv("{}/hyperparameter.csv".format(hyperDir), index=False, sep="\t")

                jobs.pop(i)
                param.pop(i)

                if(nJobs < 200):
                    param.append({key: value[0](*value[1]) for key, value in hyperParam.items()})
                    jobs.append(pool.apply_async(func, kwds=param[-1]))
    
                    if(loss != 0): 
                        nJobs += 1

def main():
    ##Parser argument
    args = parser()

    ##Set high niceness
    os.nice(19)

    if args.type == "dnn":
        config = yaml.load(open(args.config, "r"), Loader=yaml.Loader)

        # Bounded region of parameter space
        hyperParam = {
                'batchSize': [np.random.randint, (500, 4000)], 
                'lr': [np.random.randint, (-6, -2)], 
                "nNodes": [np.random.randint, (30, 300)], 
                "nLayers": [np.random.randint, (1, 10)], 
                "dropOut": [np.random.uniform, (0., 0.5)],
        }

        for channel in config["channels"]:
            hyperDir = "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].format("HyperOpt"), channel)
            optimize(partial(dnn, config, channel), hyperParam, hyperDir)


    elif args.type == "htag":
        # Bounded region of parameter space
        hyperParam = {
                'batchSize': [np.random.randint, (200, 4000)], 
                'lr': [np.random.randint, (-6, -2)], 
                "nHidden": [np.random.randint, (5, 300)], 
                "nConvFilter": [np.random.randint, (5, 300)],
                "kernelSize": [np.random.randint, (2, 200)],
                "dropOut": [np.random.uniform, (0., 0.5)],
        }

        hyperDir = "{}/DNN/Tagger/HyperOptNew".format(os.environ["CHDIR"])

        optimize(htag, hyperParam, hyperDir)
    
if __name__ == "__main__":
    main()

