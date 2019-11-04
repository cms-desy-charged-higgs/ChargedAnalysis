#!/usr/bin/env python3

from taskmanager import TaskManager

from treeread import TreeRead
from plot1d import Plot1D
from hadd import Hadd

import os
import argparse
import yaml

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--task", type=str, choices = ["Plot1D"])
    parser.add_argument("--config", type=str)

    return parser.parse_args()

def taskReturner(taskName, **kwargs):
    if kwargs["config"]:
        config =  yaml.load(open("{}/ChargedAnalysis/Workflow/config/{}.yaml".format(os.environ["CHDIR"], kwargs["config"]), "r"), Loader=yaml.Loader)

    tasks = {
        "Plot1D": lambda : plot1DTask(config),
    }

    return tasks[taskName]

def plot1DTask(config):
    allTasks = []

    for channel, conf in config.items():
        treeTasks = TreeRead.configure(config, channel)
        haddTasks = Hadd.configure(config, treeTasks, channel)
        histTasks = Plot1D.configure(config, haddTasks, channel)

        allTasks.extend(treeTasks+haddTasks+histTasks)

    return allTasks

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

