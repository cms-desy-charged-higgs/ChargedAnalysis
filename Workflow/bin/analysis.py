#!/usr/bin/env python3

from taskmanager import TaskManager

#from ChargedAnalysis.Skimming.miniskim import MiniSkim
#from ChargedAnalysis.Skimming.nanoskim import NanoSkim
#from ChargedAnalysis.Skimming.skimhadd import SkimHadd

from treeread import TreeRead
from plot1d import Plot1D

from pprint import pprint
import os
import argparse
import yaml

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--task", type=str, choices = ["MiniSkim", "NanoSkim", "Plot1D"])
    parser.add_argument("--config", type=str)

    return parser.parse_args()

def taskReturner(taskName, **kwargs):
    if kwargs["config"]:
        config =  yaml.load(open("{}/ChargedAnalysis/Workflow/config/{}".format(os.environ["CHDIR"], kwargs["config"]), "r"), Loader=yaml.Loader)

    tasks = {
        "MiniSkim": lambda : skimTask(False),
        "NanoSkim": lambda : skimTask(True),
        "Plot1D": lambda : plot1DTask(config),
    }

    return tasks[taskName]

def skimTask(isNANO):
    ##Check if MINIAOD or NANOAOD analysis
    AOD = "NANO" if isNANO else "MINI"
    Task = NanoSkim if isNANO else MiniSkim

    ##Txt with DBS dataset names
    txtFiles = [
                "{}/src/ChargedAnalysis/Skimming/data/filelists/filelist_bkg_2017_{}.txt".format(os.environ["CMSSW_BASE"], AOD),
                "{}/src/ChargedAnalysis/Skimming/data/filelists/filelist_data_2017_{}.txt".format(os.environ["CMSSW_BASE"], AOD),
                "{}/src/ChargedAnalysis/Skimming/data/filelists/filelist_signal_2017_{}.txt".format(os.environ["CMSSW_BASE"], AOD),
    ]

    ##Create tasks
    tasks = [task for f in txtFiles for task in Task.configure(f)]
    tasks.extend(SkimHadd.configure(tasks, False))

    return tasks

def plot1DTask(config):
    treeTasks = TreeRead.configure(config)
    histTasks = Plot1D.configure(treeTasks, config)

    return treeTasks + histTasks

def main():
    args = parser()

    ##Configure and get the tasks
    tasks = taskReturner(args.task, config=args.config)()

    ##Run the manager
    manager = TaskManager()
    manager.tasks = tasks
    manager.runTasks()

if __name__=="__main__":
    main()

