#!/usr/bin/env python3

from taskmanager import TaskManager

from treeread import TreeRead
from plot import Plot
from hadd import HaddPlot, HaddAppend
from treeappend import TreeAppend
from fileskim import FileSkim
from datacard import Datacard
from limit import Limit
from plotlimit import PlotLimit
from plotpostfit import PlotPostfit
from bdt import BDT
from merge import MergeCSV

import os
import argparse
import yaml
import copy
import sys
import pprint

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--task", type=str, choices = ["Plot", "BDT", "Append", "Limit"])
    parser.add_argument("--config", type=str)
    parser.add_argument("--check-output", action = "store_true")
    parser.add_argument("--no-monitor", action = "store_true")

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
        "Plot": lambda : plot(config),
        "Limit": lambda : limit(config)
    }

    return tasks[taskName]

def append(config):
    allTasks = []

    for channel in config["channels"]:
        appendTasks = TreeAppend.configure(config, channel)

        allTasks.extend(appendTasks)

    allTasks.extend(FileSkim.configure(allTasks, config["channels"]))
    allTasks.extend(HaddAppend.configure(allTasks))

    return allTasks

def bdt(config):
    allTasks = []

    for channel in config["channels"]:
        for evType, operator in zip(["Even", "Odd"], ["%", "%!"]):
            bkgConfig = copy.deepcopy(config)

            bkgConfig["processes"] = bkgConfig["backgrounds"]
            bkgConfig["x-parameters"]["all"].extend(["const_{}".format(m) for m in bkgConfig["masses"]])
            bkgConfig["cuts"].setdefault("all", []).append("evNr_{}_2".format(operator))
            bkgConfig["dir"] = bkgConfig["dir"].format(evType)

            treeTasks = TreeRead.configure(bkgConfig, channel, prefix=evType)
            haddTasks = HaddPlot.configure(bkgConfig, treeTasks, channel, prefix=evType)
            allTasks.extend(treeTasks)

            for mass in config["masses"]:
                sigConfig = copy.deepcopy(config)

                sigConfig["processes"] = [config["signal"].format(mass)]
                sigConfig["x-parameters"]["all"].append("const_{}".format(mass))
                sigConfig["cuts"].setdefault("all", []).append("evNr_{}_2".format(operator))
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
            
def limit(config):
    allTasks = []
    datacardTasks = []
    limitTasks = []
    postfitTasks = []

    for channel in config["channels"]: 
        histConfig = copy.deepcopy(config)

        histConfig["dir"] = histConfig["dir"].format("")
        histConfig["x-parameters"]["all"] = [histConfig["x-parameters"]["all"][0].format(mass) for mass in histConfig["masses"]]
        histConfig["processes"] = histConfig["backgrounds"] + [histConfig["signal"].format(mass) for mass in histConfig["masses"]]

        if histConfig["data"]:
            histConfig["processes"] += [histConfig["data"]["channel"]]

        treeTasks = TreeRead.configure(histConfig, channel) 
        haddTasks = HaddPlot.configure(histConfig, treeTasks, channel)

        allTasks.extend(treeTasks+haddTasks)

        for mass in config["masses"]:
            cardConfig = copy.deepcopy(config)
            cardConfig["dir"] = cardConfig["dir"].format(mass)
            cardConfig["discriminant"] = config["x-parameters"]["all"][0].format(mass)
            cardConfig["signal"] = cardConfig["signal"].format(mass)

            datacardTasks.extend(Datacard.configure(cardConfig, channel, mass, haddTasks))

    for mass in config["masses"]:
        limitConfig = copy.deepcopy(config)
        limitConfig["dir"] = limitConfig["dir"].format(mass)
        limitTasks.extend(Limit.configure(limitConfig, mass, datacardTasks))
        postfitTasks.extend(PlotPostfit.configure(limitConfig, mass))

    allTasks.extend(datacardTasks+limitTasks+PlotLimit.configure(histConfig)+postfitTasks)

    return allTasks

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
    with TaskManager(args.check_output, args.no_monitor) as manager:
        manager.tasks = tasks
        manager.run()

if __name__=="__main__":
    main()
