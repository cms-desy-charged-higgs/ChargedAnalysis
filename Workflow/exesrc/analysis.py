#!/usr/bin/env python3

from taskmanager import TaskManager
from configuration import *

import os
import argparse
import yaml
import copy
import sys
import numpy

from collections import defaultdict as dd

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--task", type=str, choices = ["Plot", "HTagger", "Append", "Limit", "DNN", "Slim", "Merge", "Estimate", "Stitch"])
    parser.add_argument("--config", type=str)
    parser.add_argument("--era", nargs='+', default = [""])
    parser.add_argument('--dry-run', default=False, action='store_true')
    parser.add_argument("--run-again", type=str)
    parser.add_argument("--long-condor", default=False, action='store_true')

    return parser.parse_args()

def taskReturner(taskName, config):
    tasks = {
        "Append": lambda : append(config),
        "HTagger": lambda : htagger(config),
        "DNN": lambda : dnn(config),
        "Plot": lambda : plot(config),
        "Limit": lambda : limit(config),
        "Slim": lambda : slim(config),
        "Merge": lambda: mergeskim(config),
        "Estimate": lambda : estimate(config),
        "Stitch": lambda: stitch(config),
    }

    return tasks[taskName]

def plot(config):
    tasks = []

    for channel in config["channels"]:
        for era in config["era"]:
            processes = sorted(config["processes"] + config.get("data", {}).get(channel, []))

            for process in processes:
                for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []):
                    for shift in ["Up", "Down"]:
                        treeread(tasks, config, channel, era, process, syst, shift)
                        mergeHists(tasks, config, channel, era, process, syst, shift)

            plotting(tasks, config, channel, era, processes)
            eventCount(tasks, config, channel, era, processes)

    return tasks

def mergeskim(config):
    tasks = []

    for channel in config["channels"]:
        for era in config["era"]:
            for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []):
                for shift in ["Up", "Down"]:
                    mergeSkim(tasks, config, channel, era, syst, shift)

    if "dCache" in config:
        tmpTask, mergeTasks = [t for t in tasks if "tmp" in t["dir"]], [t for t in tasks if "merged" in t["dir"]]
        sendToDCache(mergeTasks, config, os.environ["CHDIR"])

        tasks = tmpTask + mergeTasks

    return tasks

def slim(config):
    tasks = []

    for outChannel, inChannel in config["channels"].items():
        for era in config["era"]:
            for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(inChannel, []):
                for shift in ["Up", "Down"]:
                    treeslim(tasks, config, inChannel, outChannel, era, syst, shift)
                    mergeFiles(tasks, config, inChannel, outChannel, era, syst, shift)

    if "dCache" in config:
        tmpTask, mergeTasks = [t for t in tasks if "unmerged" in t["dir"]], [t for t in tasks if not "unmerged" in t["dir"]]
        sendToDCache(mergeTasks, config, os.environ["CHDIR"])

        tasks = tmpTask + mergeTasks

    return tasks

def dnn(config):
    tasks = []

    for channel in config["channels"]:
        for era in config["era"]:
            for evType in ["Even", "Odd"]:
                c = copy.deepcopy(config)

                DNN(tasks, config, channel, era, evType)

    return tasks

def append(config):
    tasks = []

    for channel in config["channels"]:
        for era in config["era"]:
            for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []):
                for shift in ["Up", "Down"]:
                    treeappend(tasks, config, channel, era, syst, shift)

    if "dCache" in config:
        sendToDCache(tasks, config, os.environ["CHDIR"])
    
    return tasks

def estimate(config):
    tasks = []

    for channel in config["channels"]:
        for era in config["era"]:
            for mass in config["charged-masses"]:
                for process in config["estimate-process"]:
                    c = copy.deepcopy(config)
                    c["dir"] = "{}/{}-Region".format(config["dir"], process).replace("{MHC}", str(mass))
                    c["cuts"].setdefault("all", []).extend([cut.format_map(dd(str, {"MHC": mass})) for cut in config["estimate-process"][process]])
                                        
                    processes = config["processes"] + config["data"][channel]

                    for procc in processes:
                        treeread(tasks, c, channel, era, procc, "", "", postFix = "{}_{}".format(process, str(mass)))
                        mergeHists(tasks, c, channel, era, procc, "", "", postFix = "{}_{}".format(process, str(mass)))

                    plotting(tasks, c, channel, era, processes, postFix = "{}_{}".format(process, str(mass)))
                    eventCount(tasks, c, channel, era, processes, postFix = "{}_{}".format(process, str(mass)))
                        
                bkgEstimation(tasks, config, channel, era, str(mass))

    return tasks

def stitch(config):
    tasks = []

    for channel in config["channels"]:
        for era in config["era"]:
            for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []):
                for shift in ["Up", "Down"]:
                    stitching(tasks, config, channel, era, syst, shift)

    return tasks

def limit(config):
    tasks = []

    for mHPlus in config["charged-masses"]:
        for mH in config["neutral-masses"]:
            conf = copy.deepcopy(config)
            conf["signal"] = config["signal"].format_map(dd(str, {"MHC": mHPlus, "MH": mH}))
            conf["discriminant"] = config["discriminant"].format_map(dd(str, {"MHC": mHPlus, "MH": mH}))
            conf["dir"] = config["dir"].replace("{MHC}", str(mHPlus)).replace("{MH}", str(mH))

            ##Produce limits for each era/channel individually
            for era in config["era"]:
                for channel in config["channels"]:
                    conf["cuts"]["all"] = [c.replace("{MHC}", str(mHPlus)) for c in config["cuts"]["all"]]
                    processes = conf["backgrounds"] + [conf["signal"]] + conf["data"][channel]

                    for procc in processes:
                        treeread(tasks, conf, channel, era, procc, "", "", postFix = "{}_{}".format(mHPlus, mH))
                        mergeHists(tasks, conf, channel, era, procc, "", "", postFix = "{}_{}".format(mHPlus, mH))

                    plotting(tasks, conf, channel, era, processes, postFix = "{}_{}".format(mHPlus, mH))

                    datacard(tasks, conf, channel, era, processes, mHPlus, mH)
                    limits(tasks, conf, channel, era, mHPlus, mH)
                    plotpostfit(tasks, conf, channel, era, mHPlus, mH)

            ##Produce limits for each era and all channel combined
            for era in config["era"]:
                mergeDataCards(tasks, conf, {"Combined": config["channels"]}, era, mHPlus, mH)
                limits(tasks, conf, "Combined", era, mHPlus, mH)
                plotpostfit(tasks, conf, "Combined", era, mHPlus, mH)


            ##Produce limits for RunII and each channel
            for channel in config["channels"]:
                mergeDataCards(tasks, conf, channel, {"RunII": config["era"]}, mHPlus, mH)
                limits(tasks, conf, channel, "RunII", mHPlus, mH)
                plotpostfit(tasks, conf, channel, "RunII", mHPlus, mH)

            ##Produce limits for RunII and all channel combined
            mergeDataCards(tasks, conf, {"Combined": config["channels"]}, {"RunII": config["era"]}, mHPlus, mH)
            limits(tasks, conf, "Combined", "RunII", mHPlus, mH)
            plotpostfit(tasks, conf, "Combined", "RunII", mHPlus, mH)

    ##Plot limits for all combination of era/channel
    conf = copy.deepcopy(config)
    conf["dir"] = config["dir"].replace("{MHC}", "").replace("{MH}", "")

    for era in config["era"]:
        for channel in config["channels"]:
            plotlimit(tasks, conf, channel, era)

    for era in config["era"]:
        plotlimit(tasks, config, "Combined", era)

    for channel in config["channels"]:
        plotlimit(tasks, config, channel, "RunII")

    plotlimit(tasks, config, "Combined", "RunII")

    return tasks

def main():
    args = parser()

    tasks = []
    taskDir = ""

    ##Read general config and update with given config
    if args.config:
        confDir = "{}/ChargedAnalysis/Workflow/config/".format(os.environ["CHDIR"])

        config = yaml.load(open("{}/general.yaml".format(confDir), "r"), Loader=yaml.Loader)
        config.update(yaml.load(open("{}/{}.yaml".format(confDir, args.config), "r"), Loader=yaml.Loader))

        taskDir = "{}/Results/{}/{}".format(os.environ["CHDIR"], args.task, config["dir"].split("/")[0])
        os.makedirs(taskDir, exist_ok = True)

        with open("{}/config.yaml".format(taskDir), "w") as conf:
            yaml.dump(config, conf, default_flow_style=False, indent=4)

        config["dir"] = "{}/Results/{}/{}".format(os.environ["CHDIR"], args.task, config["dir"])
        config["era"] = args.era

        ##Configure and get the tasks
        tasks = taskReturner(args.task, config)()

    elif args.run_again:
        taskDir = "/".join(args.run_again.split("/")[:-1])

    ##Run the manager
    manager = TaskManager(tasks = tasks, existingFlow = args.run_again, dir = taskDir, longCondor = args.long_condor)
    manager.run(args.dry_run)

if __name__=="__main__":
    main()
