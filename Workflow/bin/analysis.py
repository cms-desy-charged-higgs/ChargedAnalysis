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

    for channel, conf in config.items():
        appendTasks = TreeAppend.configure(config, channel)

        allTasks.extend(appendTasks)

    allTasks.extend(FileSkim.configure(allTasks, list(config.keys())))
    allTasks.extend(HaddAppend.configure(allTasks))

    return allTasks

def bdt(config):
    allTasks = []

    for channel in config["channels"]:
        for evType, operator in zip(["Even", "Odd"], ["%", "%!"]):
            bkgConfig = copy.deepcopy(config)

            bkgConfig["processes"] = bkgConfig["backgrounds"]
            bkgConfig["x-parameters"]["all"].extend(["const_{}".format(m) for m in bkgConfig["masses"]])
            bkgConfig["cuts"]["all"].append("evNr_{}_2".format(operator))
            bkgConfig["dir"] = bkgConfig["dir"].format(evType)

            treeTasks = TreeRead.configure(bkgConfig, channel, prefix=evType)
            haddTasks = HaddPlot.configure(bkgConfig, treeTasks, channel, prefix=evType)
            allTasks.extend(treeTasks)

            for mass in config["masses"]:
                sigConfig = copy.deepcopy(config)

                sigConfig["processes"] = [config["signal"].format(mass)]
                sigConfig["x-parameters"]["all"].append("const_{}".format(mass))
                sigConfig["cuts"]["all"].append("evNr_{}_2".format(operator))
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

    for mass in config["masses"]:
        datacardTasks = []
        haddTasks = []

        tempConf = copy.deepcopy(config)

        for channel in list(config.keys())[1:]:
            for key in tempConf[channel].keys():
                if type(tempConf[channel][key]) == str:
                    tempConf[channel][key] = tempConf[channel][key].replace("@", str(mass))

                elif type(tempConf[channel][key]) == list:
                    tempConf[channel][key] = [i.replace("@", str(mass)) for i in tempConf[channel][key]]
 
            tempConf[channel]["processes"] = tempConf[channel]["backgrounds"] + [tempConf[channel]["signal"]] + [tempConf[channel]["data"]]

            treeTasks = TreeRead.configure(tempConf, channel) 
            haddTasks.extend(HaddPlot.configure(tempConf, treeTasks, channel))
            datacardTasks.extend(Datacard.configure(tempConf, channel, mass, haddTasks))

            allTasks.extend(treeTasks)
        allTasks.extend(haddTasks+datacardTasks+Limit.configure(tempConf, mass, datacardTasks))
    allTasks.extend(PlotLimit.configure(config))

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
        manager.runTasks()

if __name__=="__main__":
    main()
