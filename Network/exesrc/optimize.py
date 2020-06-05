#!/usr/bin/env python3

from bayes_opt import BayesianOptimization, SequentialDomainReductionTransformer

import subprocess
import argparse
import os
import yaml
import pandas as pd
from multiprocessing import Pool

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("config", metavar='config', type=str, action="store", help="Yaml config with DNN configuration")
    return parser.parse_args()

def dnn(config, channel, batchSize, lr, nNodes, nLayers, dropOut):
    dnnDir = "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].format("Odd"), channel)

    hyperParam = ["--batch-size", str(batchSize), "--lr", str(10**(int(lr))), "--n-nodes", str(nNodes), "--n-layers", str(nLayers), "--drop-out", str(dropOut)]
    files = ["--sig-files", *["{d}/{sig}/{sig}.csv".format(d = dnnDir, sig = config["signal"].format(m)) for m in config["masses"]], "--bkg-files", *["{d}/{bkg}/{bkg}.csv".format(d = dnnDir, bkg = bkg) for bkg in config["backgrounds"]]]

    out = subprocess.run(["dnn", "--optimize", "--out-path", dnnDir] + hyperParam + files, stdout=subprocess.PIPE)

    return 1/float(out.stdout.decode('utf-8').split("\n")[-2].split(" ")[-1])

def optimize(config, channel):
    hyperDir = "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].format("HyperOpt"), channel)
    os.makedirs(hyperDir, exist_ok = True)
    
    # Bounded region of parameter space
    pbounds = {'batchSize': (500, 4000), 'lr': (-6, -2), "nNodes": (30, 300), "nLayers": (1, 10), "dropOut": (0., 0.5)}

    transformer = SequentialDomainReductionTransformer()
    optimizer = BayesianOptimization(f = lambda **points : dnn(config, channel, **points), pbounds = pbounds, random_state = 1, bounds_transformer = transformer)
    best = 0
    
    for i in range(40):
        optimizer.maximize(init_points=2, n_iter=5)
        
        if(optimizer.max["target"] > best):  
            best = optimizer.max["target"]
    
            frame = pd.DataFrame(columns=optimizer.max["params"].keys())    
            frame.loc[0] = optimizer.max["params"]
            frame.loc[0]["lr"] = 10**(int(frame.loc[0]["lr"]))

            frame.to_csv("{}/hyperparameter.csv".format(hyperDir), index=False, sep="\t")

def main():
    ##Parser argument
    args = parser()
    config = yaml.load(open(args.config, "r"), Loader=yaml.Loader)

    ##Set high niceness
    os.nice(19)

    pool = Pool(processes=len(config["channels"]))
    jobs = []

    for channel in config["channels"]:
         jobs.append(pool.apply_async(optimize, args=(config, channel)))

    [job.get() for job in jobs]
    
if __name__ == "__main__":
    main()

