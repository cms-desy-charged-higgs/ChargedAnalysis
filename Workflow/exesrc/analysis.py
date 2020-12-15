#!/usr/bin/env python3

from taskmanager import TaskManager
from configuration import *

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

def plot(config):
    tasks = []

    for channel in config["channels"]:
        for era in config["era"]:
            for process in config["processes"]:
                for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []):
                    for shift in ["Up", "Down"]:
                        treeread(tasks, config, channel, era, process, syst, shift)
                        mergeHists(tasks, config, channel, era, process, syst, shift)

    return tasks

def main():
    args = parser()

    ##Configure and get the tasks
    tasks = taskReturner(args.task, args.era, config=args.config)()

    ##Run the manager
    manager = TaskManager(tasks)
    manager.run()

if __name__=="__main__":
    main()
