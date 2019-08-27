#!/usr/bin/env python

from ChargedAnalysis.Workflow.taskmanager import TaskManager

from ChargedAnalysis.Skimming.miniskim import MiniSkim
from ChargedAnalysis.Skimming.nanoskim import NanoSkim
from ChargedAnalysis.Skimming.skimhadd import SkimHadd

from pprint import pprint
import os
import argparse

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--task", type=str, choices = ["MiniSkim", "NanoSkim"])

    return parser.parse_args()

def taskReturner(taskName):
    tasks = {
        "MiniSkim": lambda : skimTask(False),
        "NanoSkim": lambda : skimTask(True),
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

def main():
    args = parser()

    ##Configure and get the tasks
    tasks = taskReturner(args.task)()

    ##Run the manager
    manager = TaskManager()
    manager.tasks = tasks
    manager.runTasks()

if __name__=="__main__":
    main()

