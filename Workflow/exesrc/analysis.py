#!/usr/bin/env python3

from taskmanager import TaskManager

from treeread import TreeRead
from treeslim import TreeSlim
from plot import Plot
from hadd import HaddPlot, HaddAppend
from treeappend import TreeAppend
from datacard import Datacard
from limit import Limit
from plotlimit import PlotLimit
from plotpostfit import PlotPostfit
from bdt import BDT
from merge import MergeCSV
from dnn import DNN

import os
import argparse
import yaml
import copy
import sys
import numpy

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--task", type=str, choices = ["Plot", "BDT", "Append", "Limit", "DNN", "Slim"])
    parser.add_argument("--config", type=str)
    parser.add_argument("--check-output", action = "store_true")
    parser.add_argument("--no-http", action = "store_true")

    return parser.parse_args()

def taskReturner(taskName, **kwargs):
    confDir = "{}/ChargedAnalysis/Workflow/config/".format(os.environ["CHDIR"])

    ##Load general config
    config = yaml.load(open("{}/general.yaml".format(confDir), "r"), Loader=yaml.Loader)
    
    ##Append task specifig config
    if kwargs["config"]:
        config.update(yaml.load(open("{}/{}.yaml".format(confDir, kwargs["config"]), "r"), Loader=yaml.Loader))

    tasks = {
        "Append": lambda : append(config),
        "BDT": lambda : bdt(config),
        "DNN": lambda : dnn(config),
        "Plot": lambda : plot(config),
        "Limit": lambda : limit(config),
        "Slim": lambda : slim(config),
    }

    return tasks[taskName]

def append(config):
    allTasks = []

    for channel in config["channels"]:
        allTasks.extend(TreeAppend.configure(config, channel))

    return allTasks

def bdt(config):
    allTasks = []

    for channel in config["channels"]:
        for evType, operator in zip(["Even", "Odd"], ["divisible", "notdivisible"]):
            bkgConfig = copy.deepcopy(config)

            bkgConfig["processes"] = bkgConfig["backgrounds"]
            bkgConfig["parameters"][channel].extend(["f:n=const,v={}/t:v=0:".format(m) for m in bkgConfig["masses"]])
            bkgConfig["cuts"].setdefault("all", []).append("f:n=EvNr/c:n={},v=2".format(operator))
            bkgConfig["dir"] = bkgConfig["dir"].format(evType)

            treeTasks = TreeRead.configure(bkgConfig, channel, prefix=evType)
            haddTasks = HaddPlot.configure(bkgConfig, treeTasks, channel, prefix=evType)
            allTasks.extend(treeTasks)

            for mass in config["masses"]:
                sigConfig = copy.deepcopy(config)

                sigConfig["processes"] = [config["signal"].format(mass)]
                sigConfig["parameters"][channel].append("f:n=const,v={}/t:v=0".format(mass))
                sigConfig["cuts"].setdefault("all", []).append("f:n=EvNr/c:n={},v=2".format(operator))
                sigConfig["dir"] = sigConfig["dir"].format(evType)

                treeTasks = TreeRead.configure(sigConfig, channel, prefix=evType)
                haddTasks.extend(HaddPlot.configure(sigConfig, treeTasks, channel, prefix=evType))
                allTasks.extend(treeTasks)
        
            if("optimize" in config):
                if(evType == "Even"):
                    bdtTask = []
                    for i in range(config["optimize"]):
                        bdtTask.extend(BDT.configure(config, channel, haddTasks, evType, prefix=i))

                    allTasks.extend(MergeCSV.configure(config, bdtTask, prefix="BDT") + bdtTask)

            else:
                bdtTask = BDT.configure(config, channel, haddTasks, evType)
                allTasks.extend(bdtTask)
                    
            allTasks.extend(haddTasks)

    return allTasks       

def dnn(config):
    allTasks = []

    for channel in config["channels"]:
        for evType, operator in zip(["Even", "Odd"], ["divisible", "notdivisible"]):
            bkgConfig = copy.deepcopy(config)

            bkgConfig["processes"] = bkgConfig["backgrounds"]
            bkgConfig["cuts"].setdefault("all", []).append("f:n=EvNr/c:n={},v=2".format(operator))
            bkgConfig["dir"] = bkgConfig["dir"].format(evType)

            CSVTasks = TreeRead.configure(bkgConfig, channel, "csv", prefix=evType)
            mergeTasks = MergeCSV.configure(bkgConfig, CSVTasks, channel, prefix=evType)
            allTasks.extend(CSVTasks)

            paramDir = "{}/{}/{}/".format(os.environ["CHDIR"], bkgConfig["dir"], channel) 
            os.makedirs(paramDir, exist_ok=True)

            with open(paramDir + "/parameter.txt", "w") as param:
                for parameter in CSVTasks[0]["parameters"]:
                    param.write(parameter + "\n")

            for mass in config["masses"]:
                sigConfig = copy.deepcopy(config)

                sigConfig["processes"] = [config["signal"].format(mass)]
                sigConfig["cuts"].setdefault("all", []).append("f:n=EvNr/c:n={},v=2".format(operator))
                sigConfig["dir"] = sigConfig["dir"].format(evType)

                CSVTasks = TreeRead.configure(sigConfig, channel, "csv", prefix=evType)
                mergeTasks.extend(MergeCSV.configure(sigConfig, CSVTasks, channel, prefix=evType))
                allTasks.extend(CSVTasks)

            dnnTask = DNN.configure(config, channel, mergeTasks, evType)
            allTasks.extend(dnnTask)
                    
            allTasks.extend(mergeTasks)

    return allTasks    
            
def limit(config):
    allTasks = []
    datacardTasks = []
    limitTasks = []
    postfitTasks = []

    for channel in config["channels"]: 
        histConfig = copy.deepcopy(config)

        histConfig["dir"] = histConfig["dir"].format("")
        histConfig["parameters"]["all"] = [histConfig["parameters"]["all"][0].format(mass) for mass in histConfig["masses"]]
        histConfig["processes"] = histConfig["backgrounds"] + [histConfig["signal"].format(mass) for mass in histConfig["masses"]]

        if histConfig["data"]:
            histConfig["processes"] += [histConfig["data"]["channel"]]

        treeTasks = TreeRead.configure(histConfig, channel) 
        haddTasks = HaddPlot.configure(histConfig, treeTasks, channel)

        allTasks.extend(treeTasks+haddTasks)

        for mass in config["masses"]:
            cardConfig = copy.deepcopy(config)
            cardConfig["dir"] = cardConfig["dir"].format(mass)
            cardConfig["discriminant"] = config["discriminant"].format(mass)
            cardConfig["signal"] = cardConfig["signal"].format(mass)

            datacardTasks.extend(Datacard.configure(cardConfig, channel, mass, haddTasks))

    for mass in config["masses"]:
        limitConfig = copy.deepcopy(config)
        limitConfig["dir"] = limitConfig["dir"].format(mass)
        limitTasks.extend(Limit.configure(limitConfig, mass, datacardTasks))
      #  postfitTasks.extend(PlotPostfit.configure(limitConfig, mass))

    allTasks.extend(datacardTasks+limitTasks+PlotLimit.configure(histConfig)+postfitTasks)

    return allTasks

def slim(config):
    return TreeSlim.configure(config)

def plot(config):
    allTasks = []

    for channel in config["channels"]:
        treeTasks = TreeRead.configure(config, channel)
        haddTasks = HaddPlot.configure(config, treeTasks, channel)
        histTasks = Plot.configure(config, haddTasks, channel)

        allTasks.extend(treeTasks+haddTasks+histTasks)

    return allTasks

def main():
    args = parser()

    ##Configure and get the tasks
    tasks = taskReturner(args.task, config=args.config)()

    ##Run the manager
    with TaskManager(args.check_output, args.no_http) as manager:
        manager.tasks = tasks
        manager.run()

if __name__=="__main__":
    main()