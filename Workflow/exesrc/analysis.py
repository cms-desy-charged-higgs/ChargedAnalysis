#!/usr/bin/env python3

from taskmanager import TaskManager

from treeread import TreeRead
from treeslim import TreeSlim
from treeappend import TreeAppend

from estimate import Estimate

from plot import Plot
from plotlimit import PlotLimit
from plotpostfit import PlotPostfit

from datacard import Datacard
from combineCards import CombineCards
from limit import Limit

from dnn import DNN
from htagger import HTagger

from mergeskim import MergeSkim
from merge import Merge

from dcache import DCache

import os
import argparse
import yaml
import copy
import sys
import numpy

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--task", type=str, choices = ["Plot", "HTagger", "Append", "Limit", "DNN", "Slim", "Merge", "Estimate"])
    parser.add_argument("--config", type=str)
    parser.add_argument("--era", nargs='+', default = [""])
    parser.add_argument("--check-output", action = "store_true")
    parser.add_argument("--no-http", action = "store_true")

    return parser.parse_args()

def taskReturner(taskName, era, **kwargs):
    confDir = "{}/ChargedAnalysis/Workflow/config/".format(os.environ["CHDIR"])

    ##Load general config
    config = yaml.load(open("{}/general.yaml".format(confDir), "r"), Loader=yaml.Loader)
    
    ##Append task specifig config
    if kwargs["config"]:
        config.update(yaml.load(open("{}/{}.yaml".format(confDir, kwargs["config"]), "r"), Loader=yaml.Loader))

    config["era"] = era

    tasks = {
        "Append": lambda : append(config),
        "HTagger": lambda : htagger(config),
        "DNN": lambda : dnn(config),
        "Plot": lambda : plot(config),
        "Limit": lambda : limit(config),
        "Slim": lambda : slim(config),
        "Merge": lambda: merge(config),
        "Estimate": lambda : estimate(config),
    }

    return tasks[taskName]

def append(config):
    allTasks = []
    dCacheTasks = []

    for era in config["era"]:
        for channel in config["channels"]:
            allTasks.extend(TreeAppend.configure(config, channel, era))

    if "dCache"  in config:
        dCacheTasks = DCache.configure(config, allTasks)

    return allTasks + dCacheTasks 

def dnn(config):
    allTasks = []

    for era in config["era"]:
        for evType, operator in zip(["Even", "Odd"], ["divisible", "notdivisible"]):
            c = copy.deepcopy(config)
            c["cuts"].setdefault("all", []).append("f:n=EvNr/c:n={},v=2".format(operator))

            dnnTask = DNN.configure(c, evType, config["masses"], era)
            allTasks.extend(dnnTask)

    return allTasks

def htagger(config):
    allTasks = []

    for era in config["era"]:
        for evType, operator in zip(["Even", "Odd"], ["divisible", "notdivisible"]):
            c = copy.deepcopy(config)
            c["cuts"].setdefault("all", []).append("f:n=EvNr/c:n={},v=2".format(operator))

            dnnTask = HTagger.configure(c, evType, era)
            allTasks.extend(dnnTask)

    return allTasks    
            
def limit(config):
    allTasks = []
    cardTasks = []

    for era in config["era"]:
        for channel in config["channels"]:
            conf = copy.deepcopy(config)
            conf["parameters"][channel] = [param.replace("[MHC]", str(mHC)) for param in config["parameters"][channel] for mHC in conf["charged-masses"]]
            conf["processes"] = conf["backgrounds"] + [config["signal"].replace("[MHC]", str(mHC)).replace("[MH]", str(mh)) for mHC in conf["charged-masses"] for mh in conf["neutral-masses"]]

            if conf["data"]:
                conf["processes"] += [conf["data"]["channel"]]

            treeTasks = TreeRead.configure(conf, channel, era) 
            haddTasks = Merge.configure(conf, treeTasks, "plot", era = era, channel = channel)
            datacardTasks = Datacard.configure(conf, channel, era, haddTasks)
            limitTasks = Limit.configure(conf, channel, era, datacardTasks)
    
            cardTasks.extend(datacardTasks)
            allTasks.extend(treeTasks+haddTasks+datacardTasks+limitTasks)

    combinedCardsTask = CombineCards.configure(config, cardTasks)
    for era in config["era"]:
        allTasks.extend(Limit.configure(conf, "Combined", era, combinedCardsTask))
    allTasks.extend(Limit.configure(conf, "Combined", "RunII", combinedCardsTask))
    allTasks.extend(PlotLimit.configure(config, allTasks))

    return allTasks + combinedCardsTask

def slim(config):
    slimTasks = TreeSlim.configure(config)
    mergeTasks = Merge.configure(config, slimTasks, "slim")
    dCacheTasks = DCache.configure(config, mergeTasks)

    return slimTasks + mergeTasks + dCacheTasks

def merge(config):
    mergeTasks = MergeSkim.configure(config)
    dCacheTasks = []

    if "dCache"  in config:
        dCacheTasks = DCache.configure(config, [task for task in mergeTasks if not "tmp" in task["dir"]])

    return mergeTasks + dCacheTasks
    
def estimate(config):
    allTasks = []

    for era in config["era"]:
        for channel in config["channels"]:
            for mass in config["charged-masses"]:
                haddTasks = [] 
            
                for process in config["estimate-process"]:
                    c = copy.deepcopy(config)
                    c["dir"] = "{}/{}-Region".format(config["dir"], process).replace("[MHC]", str(mass))
                    c.setdefault("cuts", {}).setdefault(channel, config["estimate-process"][process].get(channel, []) + config["estimate-process"][process].get("all", []))
                    c["cuts"] = {key: [cut.replace("[MHC]", str(mass)) for cut in c["cuts"][key]] for key in c["cuts"]}
                    
                    treeTasks = TreeRead.configure(c, channel, era, prefix=process + str(mass))
                    haddTasks.extend(Merge.configure(c, treeTasks, "plot", era = era, channel = channel, prefix=process + str(mass)))
                    allTasks.extend(treeTasks)
                    
                estimateTasks = Estimate.configure(config, haddTasks, era = era, channel = channel, mass = str(mass))
                allTasks.extend(haddTasks + estimateTasks)
   
    return allTasks

def plot(config):
    allTasks = []

    for era in config["era"]:
        for channel in config["channels"]:
            treeTasks = TreeRead.configure(config, channel, era)
            haddTasks = Merge.configure(config, treeTasks, "plot", era = era, channel = channel)
            histTasks = Plot.configure(config, haddTasks, channel, era)

            allTasks.extend(treeTasks+haddTasks+histTasks)

    return allTasks

def main():
    args = parser()

    ##Configure and get the tasks
    tasks = taskReturner(args.task, args.era, config=args.config)()

    ##Run the manager
    with TaskManager(args.check_output, args.no_http) as manager:
        manager.tasks = tasks
        manager.run()

if __name__=="__main__":
    main()
