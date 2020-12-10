#!/usr/bin/env python3

import subprocess
import argparse
import os
import yaml
import pandas as pd
import time
import numpy as np
import sys
from multiprocessing import Pool, cpu_count

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--dir", type=str, action="store", required = True, help="Dir with DNN information")
    parser.add_argument("--channels", type=str, nargs = "+", required = True, help="Channel name")
    parser.add_argument("--era", type=str, nargs = "+", required = True, help="Era")
    parser.add_argument("--type", type=str, action="store", required = True, choices = ["DNN", "HTag"], help="Which DNN to optimize")
    return parser.parse_args()

def prepareJobs(oldExeFile, hyperParam):
    jobDirs, params = [], []
    N = 200

    condorSub = [
            "universe = vanilla",
            "executable = $(DIR)/run.sh",
            "getenv = True",
            "log = $(DIR)/log.txt",
            "error = $(DIR)/err.txt",
            "output = $(DIR)/out.txt",
    ]
           
    ##Write submit file
    with open("/tmp/condor.sub", "w") as condFile:
        for line in condorSub:
            condFile.write(line + "\n")
    
    for i in range(N):
        tmpDir = "Tmp/tmpDir{}".format(time.time_ns())
        os.makedirs(tmpDir, exist_ok = True)
        jobDirs.append(tmpDir)
        
        param = {key: value[0](*value[1]) for key, value in hyperParam.items()}
        params.append(param)

        with open(tmpDir + "/run.sh", "w") as exe:
            with open(oldExeFile, "r") as oldExe:
                while(True):
                    line = oldExe.readline()
                    
                    if not line:
                        break
            
                    if not "opt-param" in line:
                        exe.write(line)

                    else:
                        oldExe.readline()

            for p, v in param.items():        
                exe.write("{}--{} \\".format(2*" ", p))
                exe.write("\n")
                exe.write("{}{} \\".format(6*" ", v if p != "lr" else 10**v))
                exe.write("\n")
                
            exe.write("--optimize\n")

        subprocess.run("chmod a+rx {}/run.sh".format(tmpDir), shell = True)
    
    subprocess.run("condor_submit /tmp/condor.sub -queue DIR in " + " ".join(jobDirs), shell = True)

    return jobDirs, params

def optimize(outDir, jobDirs, params):
    os.makedirs(outDir, exist_ok = True)

    bestLoss = 1e7
    N = len(jobDirs)

    while(jobDirs):
        for idx, jobDir in enumerate(jobDirs):
            if not os.path.exists("{}/log.txt".format(jobDir)): continue
        
            with open("{}/log.txt".format(jobDir), "r") as log:
                if "(return value" in log.read():
                    with open("{}/out.txt".format(jobDir), "r") as out:
                        try:
                            loss = float(out.read().split("\n")[-2])
                        
                        except:
                            loss = 1e7
       
                        if loss < abs(bestLoss):
                            bestLoss = loss
                            p = params[idx]
                            p["lr"] = 10**(p["lr"])
                                 
                            frame = pd.DataFrame.from_dict({k: [v] for (k, v) in p.items()})    
                            frame.to_csv("{}/hyperparameter.csv".format(outDir), index=False, sep="\t")
                            
                    jobDirs.pop(idx)
                    params.pop(idx)

def main():
    ##Parser argument
    args = parser()

    ##Set high niceness
    pool = Pool(cpu_count())
    jobs = []

    if args.type == "DNN":
        # Bounded region of parameter space
        hyperParam = {
                'batch-size': [np.random.randint, (200, 4000)], 
                'lr': [np.random.randint, (-6, -2)], 
                "n-nodes": [np.random.randint, (30, 500)], 
                "n-layers": [np.random.randint, (1, 10)], 
                "drop-out": [np.random.uniform, (0., 0.5)],
        }

        for era in args.era:
            for channel in args.channels:
                outDir = "{}/HyperOpt/{}/{}".format(args.dir, channel, era)
                oldExe = "{}/Even/{}/{}/run.sh".format(args.dir, channel, era)
                
                jobDirs, params = prepareJobs(oldExe, hyperParam)
                jobs.append(pool.apply_async(optimize, args = (outDir, jobDirs, params)))
                
        [job.get() for job in jobs]
        
        subprocess.run("command rm -rf Tmp", shell = True)
        
    elif args.type == "HTag":
        # Bounded region of parameter space
        hyperParam = {
                'batch-size': [np.random.randint, (200, 4000)], 
                'lr': [np.random.randint, (-6, -2)], 
                "drop-out": [np.random.uniform, (0., 0.5)],
                "n-hidden": [np.random.randint, (5, 300)], 
                "n-convfilter": [np.random.randint, (5, 300)],
                "n-kernelsize": [np.random.randint, (2, 200)],
        }

        outDir = "{}/HyperOpt".format(args.dir)
        oldExe = "{}/Even/{}/run.sh".format(args.dir, args.era)

        optimize(hyperParam, outDir, oldExe)
    
if __name__ == "__main__":
    main()
    print()

