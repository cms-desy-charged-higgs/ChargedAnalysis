from task import Task

import yaml
import os
import copy
import csv
import time
import subprocess
import glob
import numpy as np

import ROOT

from collections import defaultdict as dd

def sendToDCache(inFile, relativPath):
    dCachePath = "/pnfs/desy.de/cms/tier2/store/user/dbrunner/"

    command = "sendToDCache \\\n{}--input-file \\\n{}{} \\\n{}--dcache-path \\\n{}{} \\\n{}--relative-path \\\n{}{}\n".format(2*" ", 6*" ", inFile, 2*" ", 6*" ", dCachePath, 2*" ", 6*" ", relativPath)

    return command

def treeread(tasks, config, channel, era, process, syst, shift, mHPlus = "200", postFix = ""):
    ##Dic with process:filenames 
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "Nominal" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != "Nominal"): 
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    skimBaseDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"])
        
    ## Scale systematics
    scaleSysts = config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, []) if process not in ["SingleE", "SingleMu"] and syst == "Nominal" else []

    fileNames = []

    ##Data driven scale factors           
    bkgYldFactor, bkgYldFactorSyst, bkgType = "", [], "Misc"

    if process != "QCD":
        if "bkg-yield-factor" in config:
            if "Single" not in process and "HPlus" not in process:
                bkgYldFactor = "{}/{}".format(os.environ["CHDIR"], config["bkg-yield-factor"].format_map(dd(str, {"C": channel, "E": era, "MHC": mHPlus, "S": systName})))

                with open(bkgYldFactor) as f:
                    reader = csv.reader(f, delimiter='\t')

                    for row in reader:
                        if process in row:
                            bkgType = process
                            break

                for scaleSyst in scaleSysts:
                    for scaleShift in ["Up", "Down"]:
                        bkgYldFactorSyst.append("{}/{}".format(os.environ["CHDIR"], config["bkg-yield-factor"].format_map(dd(str, {"C": channel, "E": era, "MHC": mHPlus, "SYS": scaleSyst, "SHIFT": scaleShift}))))

    procName = config["data"][channel][0] if (config.get("qcd-estimate", False) and process == "QCD") else process

    ##List of filenames for each process
    for proc in os.listdir(skimBaseDir):
        for processName in processes[procName]:
            if proc.startswith(processName) and not "_ext" in proc:
                fileNames.append("{}/merged/{}.root".format(skimDir.format_map(dd(str, {"C": channel, "E": era, "P": proc, "S": systName})), proc))

    ##List of parameters
    parameters = config["parameters"].get("all", []) + config["parameters"].get(channel, [])
    if config.get("estimate-QCD", False):
        parameters = parameters +  [p + "/" +  config.get("estimate-QCD", "") for p in parameters]

    nJobs = 0

    for fileName in fileNames:
        f = ROOT.TFile.Open(fileName)
        t = f.Get(channel)   
        nEvents = t.GetEntries()
        f.Close()

        eventRanges = [[i if i == 0 else i+1, i+config["n-events"] if i+config["n-events"] <= nEvents else nEvents] for i in range(0, nEvents, config["n-events"])]

        outDir, systDirs = {}, {}

        ##Configuration for treeread Task
        for start, end in eventRanges:
            ##Configuration for treeread Task
            task = {
                "name": "TreeRead_{}_{}_{}_{}_{}_{}".format(channel, era, process, systName, nJobs, postFix),
                "executable": "hist",
                "dir": config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": "Executables", "P": process, "S": systName})) + "/unmerged/{}".format(nJobs),
                "run-mode": config["run-mode"],
                "arguments": {
                    "filename": fileName,
                    "parameters": parameters,
                    "regions": config.get("regions", ["Default"]),
                    "out-file": "{}.root".format(process),
                    "channel": channel,
                    "scale-systs": scaleSysts,
                    "bkg-yield-factor": bkgYldFactor,
                    "bkg-yield-factor-syst": bkgYldFactorSyst,
                    "bkg-type": bkgType,
                    "era": era,
                    "event-start": start,
                    "event-end": end
                }
            }

            if config.get("qcd-estimate", False) and process == "QCD":
                task["arguments"]["fake-rate"] = os.environ["CHDIR"] + "/" + config["qcd-estimate"]["rates"].format_map(dd(str, {"C": channel, "E": era, "R": "fake", "S": systName}))
                task["arguments"]["prompt-rate"] = os.environ["CHDIR"] + "/" + config["qcd-estimate"]["rates"].format_map(dd(str, {"C": channel, "E": era, "R": "prompt", "S": systName}))

            for region in config.get("regions", ["Default"]):
                task["arguments"]["{}-out-dir".format(region)] = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": region, "P": process, "S": systName})) + "/unmerged/{}".format(nJobs)
                task["arguments"]["{}-syst-dirs".format(region)] = [task["arguments"]["{}-out-dir"].replace(systName, "{}{}".format(scaleSyst, scaleShift)) for scaleSyst in scaleSysts for scaleShift in ["Up", "Down"]]
                task["arguments"]["{}-cuts".format(region)] = config["cuts"].get("all", []) + (config["cuts"][channel][region] if region in config["cuts"][channel] else config["cuts"].get(channel, []))

                if config.get("qcd-estimate", False) and process == "QCD":
                    lRegion = "Loose-{}".format(region)

                    task["arguments"]["{}-out-dir".format(lRegion)] = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": lRegion, "P": process, "S": systName})) + "/unmerged/{}".format(nJobs),
                    task["arguments"]["{}-cuts".format(lRegion)] = [c for c in task["arguments"]["{}-cuts".format(region)] if not "replaceForFR" in c]
                    task["arguments"]["{}-cuts".format(lRegion)].extend(config["qcd-estimate"]["cuts"][channel])
                    task["arguments"]["regions"].append(lRegion)

            tasks.append(Task(task, "--"))

            nJobs += 1

def mergeHists(tasks, config, channel, era, process, syst, shift, postFix = ""):
    inputFiles, dependencies = {}, []

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "Nominal" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != "Nominal"): 
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    for t in tasks:
        if "TreeRead_{}_{}_{}".format(channel, era, process) in t["name"] and postFix in t["name"]:
            if systName in t["name"]:
                dependencies.append(t["name"])

                for region in config.get("regions", ["Default"]):
                    inputFiles.setdefault(region, []).append("{}/{}".format(t["arguments"]["{}-out-dir".format(region)], t["arguments"]["out-file"]))
                
            elif syst in t["arguments"]["scale-systs"]:
                dependencies.append(t["name"])

                for region in config.get("regions", ["Default"]):
                    inputFiles.setdefault(region, []).append("{}/{}".format(t["arguments"]["{}-out-dir".format(region)].replace("Nominal", systName), t["arguments"]["out-file"]))

    for region in config.get("regions", ["Default"]):
        ##Output directory
        outDir = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": region, "P": process, "S": systName})) + "/merged/"

        task = {
            "name": "MergeHists_{}_{}_{}_{}_{}_{}".format(channel, era, region, process, systName, postFix),
            "executable": "merge",
            "dir": outDir,
            "dependencies": dependencies,
            "process": process,
            "arguments": {
                "input-files": inputFiles[region],
                "out-file": "{}/{}.root".format(outDir, process),
                "exclude-objects": [], 
            }
        }

        tasks.append(Task(task, "--"))

def treeIndex(tasks, config, channel, era, process, syst, shift, postFix = ""):
    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "Nominal" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != "Nominal"): 
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    skimBaseDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"])

    ##Dic with process:filenames 
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))
    fileNames = []
    procNames = []

    ##List of filenames for each process
    for proc in os.listdir(skimBaseDir):
        for processName in processes[process]:
            if proc.startswith(processName) and not "_ext" in proc:
                fileNames.append("{}/merged/{}.root".format(skimDir.format_map(dd(str, {"C": channel, "E": era, "P": proc, "S": systName})), proc))
                procNames.append(proc)
        
    nJobs = 0

    for procName, fileName in zip(procNames, fileNames):
        f = ROOT.TFile.Open(fileName)
        t = f.Get(channel)   
        nEvents = t.GetEntries()
        f.Close()

        eventRanges = [[i if i == 0 else i+1, i+config["n-events"] if i+config["n-events"] <= nEvents else nEvents] for i in range(0, nEvents, config["n-events"])]

        outDir, systDirs = {}, {}

        ##Configuration for treeread Task
        for start, end in eventRanges:
            ##Configuration for treeread Task
            task = {
                "name": "TreeIndex_{}_{}_{}_{}_{}_{}".format(channel, era, procName, systName, nJobs, postFix),
                "executable": "treeIndex",
                "dir": config["dir"].format_map(dd(str, {"D": "Index", "C": channel, "E": era, "R": "Executables", "P": process, "S": systName})) + "/unmerged/{}".format(nJobs),
                "run-mode": config["run-mode"],
                "arguments": {
                    "filename": fileName,
                    "regions": config.get("regions", ["Default"]),
                    "out-file": fileName.split("/")[-1].replace("root", "csv"),
                    "channel": channel,
                    "era": era,
                    "event-start": start,
                    "event-end": end
                }
            }

            for region in config.get("regions", ["Default"]):
                task["arguments"]["{}-out-dir".format(region)] = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": region, "P": process, "S": systName})) + "/unmerged/{}".format(nJobs)
                task["arguments"]["{}-cuts".format(region)] = config["cuts"].get("all", []) + (config["cuts"][channel][region] if region in config["cuts"][channel] else config["cuts"].get(channel, []))

            tasks.append(Task(task, "--"))

            nJobs += 1

def mergeCSV(tasks, config, channel, era, process, syst, shift, postFix = ""):
    inputFiles, dependencies = {}, {}

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "Nominal" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != "Nominal"): 
        return

    skimBaseDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"])

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    ##Dic with process:filenames 
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))
    procNames = []

    ##List of filenames for each process
    for proc in os.listdir(skimBaseDir):
        for processName in processes[process]:
            if proc.startswith(processName) and not "_ext" in proc:
                procNames.append(proc)

    for t in tasks:
        for proc in procNames:
            if "TreeIndex_{}_{}_{}".format(channel, era, proc) in t["name"] and postFix in t["name"]:
                if systName in t["name"]:
                    dependencies.setdefault(proc, []).append(t["name"])

                    for region in config.get("regions", ["Default"]):
                        inputFiles.setdefault(proc, {}).setdefault(region, []).append("{}/{}".format(t["arguments"]["{}-out-dir".format(region)], t["arguments"]["out-file"]))

                elif not "Up" in t["name"] and not "Down" in t["name"] and syst == "Nominal":
                    dependencies.setdefault(proc, []).append(t["name"])

                    for region in config.get("regions", ["Default"]):
                        inputFiles.setdefault(proc, {}).setdefault(region, []).append("{}/{}".format(t["arguments"]["{}-out-dir".format(region)].replace("unmerged/", "{}/unmerged/".format(systName)), t["arguments"]["out-file"]))

    ##Output directory
    for proc in procNames:
        for region in config.get("regions", ["Default"]):
            outDir = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": region, "P": proc, "S": systName})) + "/merged/"

            task = {
                "name": "MergeCSV_{}_{}_{}_{}_{}_{}".format(channel, era, region, proc, systName, postFix),
                "executable": "mergeCSV",
                "dir": outDir,
                "dependencies": dependencies[proc],
                "arguments": {
                    "input-files": inputFiles[proc][region],
                    "out-file": "{}/{}.csv".format(outDir, proc),
                }
            }

            tasks.append(Task(task, "--"))

def plotting(tasks, config, channel, era, processes, postFix = ""):
    dependencies = {}
    bkgProcesses, sigProcesses = {}, {}
    bkgFiles, sigFiles = {}, {}
    data, dataFile = "", {}

    processes.sort()

    systematics = config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []) + config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, [])

    for t in tasks:
        for process in processes:
            for i, region in enumerate(config.get("regions", ["Default"])):
                if "MergeHists_{}_{}_{}_{}_".format(channel, era, region, process) in t["name"] and postFix in t["name"]:
                    dependencies.setdefault(region, []).append(t["name"])

                elif "QCDEstimate_{}_{}".format(channel, era) in t["name"] and postFix in t["name"] and not "QCD-" in region and process == "QCD":
                    dependencies.setdefault(region, []).append(t["name"])

                else:
                    continue

                if "Single" in process:
                    data = process
                    dataFile[region] = t["arguments"]["out-file"]
                    continue
                        
                for syst in systematics:
                    for shift in ["Up", "Down"]:
                        if syst == "Nominal" and shift == "Down":
                            continue

                        systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
                        systCheck = systName in t["name"] if syst != "" else not "Up" in t["name"] and not "Down" in t["name"]

                        if "HPlus" in process and systName in t["name"] and systCheck:
                            if syst == "Nominal":
                                sigProcesses.setdefault(region, []).append(process)
                            sigFiles.setdefault(region, {}).setdefault(systName, []).append(t["arguments"]["out-file"])

                        elif not "Single" in process and systName in t["name"] and systCheck:
                            if syst == "Nominal":
                                bkgProcesses.setdefault(region, []).append(process)
                            bkgFiles.setdefault(region, {}).setdefault(systName, []).append(t["arguments"]["out-file"])

    for region in config.get("regions", ["Default"]):
        outDir = config["dir"].format_map(dd(str, {"D": "PDF", "C": channel, "E": era, "R": region}))
        webDir = outDir.replace("Results/", "WebPlotting/Plots/").replace("PDF", "")

        task = {
            "name": "Plot_{}_{}_{}_{}".format(channel, era, region, postFix), 
            "executable": "plot",
            "dir": outDir,
            "dependencies": dependencies[region],
            "arguments": {
                "channel": channel,
                "era": era,
                "bkg-processes": bkgProcesses.get(region, []),
                "bkg-files": bkgFiles.get(region, {}).get("Nominal", []),
                "sig-processes": sigProcesses.get(region, []),
                "sig-files": sigFiles.get(region, {}).get("Nominal", []),
                "data": data,
                "data-file": dataFile.get(region, ""),
                "out-dirs": [outDir, webDir] if webDir != "" else [outDir],
                "systematics": systematics
            }
        }

        if config.get("plot-1D", True):
            task["arguments"]["plot-1D"] = ""

        if config.get("plot-2D", False):
            task["arguments"]["plot-2D"] = ""

        for syst in systematics[1:]:
            for shift in ["Up", "Down"]:
                systName = "{}{}".format(syst, shift)

                if task["arguments"]["bkg-files"]:
                    task["arguments"]["bkg-files-{}{}".format(syst, shift)] = bkgFiles[region][systName]

                if task["arguments"]["sig-files"]:
                    task["arguments"]["sig-files-{}{}".format(syst, shift)] = sigFiles[region][systName]

        tasks.append(Task(task, "--"))

def treeappend(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    skimBaseDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))

    for fileName in os.listdir(skimBaseDir):
        if "Run2" in fileName and syst != "":
            continue

        ##Ignore extensions
        if "_ext" in fileName:
            continue

        d = config["dir"].format_map(dd(str, {"C": channel, "E": era, "P": fileName, "S": systName}))
        skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era, "P": fileName, "S": systName})))

        ##Configuration for treeread Task
        task = {
            "name": "Append_{}_{}_{}_{}_{}".format(channel, era, fileName, systName, postFix),
            "dir":  d,
            "executable": "treeappend",
            "run-mode": config["run-mode"],
            "arguments": {
                "out-file": "{}/merged/{}.root".format(skimDir, fileName),
                "tree-name": channel,
                "functions": list(config["functions"].keys()),
                "era": era,
            }
        }

        for function in task["arguments"]["functions"]:
            for info in config["functions"][function]:
                task["arguments"]["{}-{}".format(function, info)] = config["functions"][function][info]

        tasks.append(Task(task, "--"))

def stitching(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))

    filesToSwitch = {}

    for fileName in os.listdir(skimDir):
        ##Ignore extensions
        if "ext" in fileName:
            continue

        for name, filePrefixes in config["files"].items():
            for f in filePrefixes:
                if fileName.startswith(f):
                    filesToSwitch.setdefault(name, []).append("{}/{}/{}/merged/{}.root".format(skimDir, fileName, systName, fileName))

    for name, fileNames in filesToSwitch.items():
        task = {
            "name": "Stich_{}_{}_{}_{}_{}".format(channel, era, name, systName, postFix),
            "dir": "{}/{}/{}".format(config["dir"].format_map(dd(str, {"C": channel, "E": era})), name, systName),
            "executable": "stitching",
            "run-mode": config["run-mode"],
            "arguments": {
                "input-files": fileNames,
            }
        }

        beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
        afterExe = [sendToDCache(f, "/".join(f.replace(os.environ["CHDIR"], "").split("/")[:-1])) for f in fileNames]

        tasks.append(Task(task, "--", beforeExe = beforeExe, afterExe = afterExe))

def fakerate(tasks, config, channel, era, mode, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    outDir = config["dir"].format_map(dd(str, {"D": "Rates", "C": channel, "E": era})) + "/" + mode + "/" + systName
    data = config["data"].get(channel, ["data"])[0]

    depFiles, depTasks = {}, []

    for t in tasks:
        if "MergeHists_{}_{}_".format(channel, era) in t["name"] and ("{}-loose".format(mode) in t["name"] or "{}-tight".format(mode) in t["name"]):
            region = "{}-loose".format(mode) if "{}-loose".format(mode) in t["name"] else "{}-tight".format(mode)

            if systName in t["name"] and systName != "Nominal":
                depTasks.append(t)
                depFiles.setdefault(region, {}).setdefault(t["process"], t["arguments"]["out-file"])

            elif data in t["name"]:
                depTasks.append(t)
                depFiles.setdefault(region, {}).setdefault("data", t["arguments"]["out-file"])

            elif syst == "Nominal" and not "Up" in t["name"] and not "Down" in t["name"]:
                depTasks.append(t)
                depFiles.setdefault(region, {}).setdefault(t["process"], t["arguments"]["out-file"])

    task = {
        "name": "Fakerate_{}_{}_{}_{}_{}".format(channel, era, mode, systName, postFix),
        "dir": outDir,
        "executable": "estimateFR",
        "dependencies": [t["name"] for t in depTasks],
        "arguments": {
            "mode": mode,
            "out-dir": outDir,
            "process":  config["proc-usage"]["use"][mode],
            "processes-to-remove":  config["proc-usage"]["remove"][mode],
        }
    }

    plotTask = {
        "name": "PlotFakerate_{}_{}_{}_{}_{}".format(channel, era, mode, systName, postFix),
        "dir": outDir.replace("Rates", "ResultPDF"),
        "executable": "plotFR",
        "dependencies": [task["name"]],
        "arguments": {
            "mode": mode,
            "out-dir": [outDir.replace("Rates", "ResultPDF"), outDir.replace("Results", "WebPlotting/Plots").replace("Rates", "")],
            "era": era,
            "channel": channel,
            "input-file": "{}/{}rate.root".format(outDir, mode)
        }
    }
          
    for process in [task["arguments"]["process"]] + task["arguments"]["processes-to-remove"]:
        task["arguments"]["{}-{}-file".format("{}-loose".format(mode), process)] = depFiles["{}-loose".format(mode)][process]
        task["arguments"]["{}-{}-file".format("{}-tight".format(mode), process)] = depFiles["{}-tight".format(mode)][process]
    
    tasks.append(Task(task, "--"))
    tasks.append(Task(plotTask, "--"))

def QCDEstimation(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    outDir = config["dir"].format_map(dd(str, {"D": "QCDEstimation", "C": channel, "E": era})) + "/" + systName
    data = config["data"][channel][0]

    depTasks = []
    depFiles = {}

    regions = [r.split("-")[-1] for r in config["regions"] if "QCD" in r]

    for t in tasks:
        for region in regions:
            if "MergeHists_{}_{}_".format(channel, era) in t["name"] and "QCD-{}".format(region) in t["name"]:
                if systName in t["name"] and systName != "Nominal":
                    depTasks.append(t)
                    depFiles.setdefault(region, {}).setdefault(t["process"], t["arguments"]["out-file"])

                elif data in t["name"]:
                    depTasks.append(t)
                    depFiles.setdefault(region, {}).setdefault("data", t["arguments"]["out-file"])

                elif syst == "Nominal" and not "Up" in t["name"] and not "Down" in t["name"]:
                    depTasks.append(t)
                    depFiles.setdefault(region, {}).setdefault(t["process"], t["arguments"]["out-file"])

                break

    task = {
        "name": "QCDEstimate_{}_{}_{}_{}".format(channel, era, systName, postFix),
        "dir": outDir,
        "executable": "estimateQCD",
        "dependencies": [t["name"] for t in depTasks],
        "arguments": {
            "out-file": "{}/{}.root".format(outDir, "tf" if config.get("only-tf", False) else "QCD"),
            "processes": [proc for proc in config["processes"] if proc != "QCD"] + ["data"],
            "regions": regions,
            "bins": config.get("bins", []),
        }
    }

    if config.get("only-tf", False):
        task["arguments"]["only-tf"] = ""
          
    for process in task["arguments"]["processes"]:
        for region in regions:
            task["arguments"]["{}-{}-file".format(region, process)] = depFiles[region][process]
    
    tasks.append(Task(task, "--"))

def bkgEstimation(tasks, config, channel, era, mass, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    outDir = config["dir"].format_map(dd(str, {"D": "ScaleFactor", "C": channel, "E": era, "MHC": mass})) + "/" + systName
    data = config["data"][channel][0]

    depTasks = []

    for t in tasks:
        if "MergeHists_{}_{}_".format(channel, era) in t["name"]:
            if mass in t["name"]:
                if systName in t["name"] and systName != "Nominal":
                    depTasks.append(t)

                elif data in t["name"]:
                    depTasks.append(t)

                elif syst == "Nominal" and not "Up" in t["name"] and not "Down" in t["name"]:
                    depTasks.append(t)

    task = {
        "name": "Estimate_{}_{}_{}_{}_{}".format(channel, era, mass, systName, postFix),
        "dir": outDir,
        "executable": "estimate",
        "dependencies": [t["name"] for t in depTasks],
        "arguments": {
            "out-dir": outDir,
            "processes": list(config["estimate-process"].keys()),
            "parameter": config["parameter"],
        }
    }
          
    for process in config["estimate-process"].keys():
        fileNames = [t["arguments"]["out-file"] for t in depTasks if "{}-Region".format(process) in t["dir"]]
                  
        for p in reversed(config["estimate-process"]):
            for idx, f in enumerate(fileNames):
                if p in f.split("/")[-1]:
                    fileNames.pop(idx)
                    fileNames.insert(0, f)
                        
                elif data in f.split("/")[-1]:
                    dataFile = fileNames.pop(idx)
                        
        task["arguments"]["{}-bkg-files".format(process)] = fileNames
        task["arguments"]["{}-data-file".format(process)] = dataFile

    tasks.append(Task(task, "--"))

def mergeSkim(tasks, config, channel, era, syst, shift, postFix = ""):
    skimDir = config["skim-dir"].format_map(dd(str, {"C": channel, "E": era}))
    absSkimDir = "{}/{}".format(os.environ["CHDIR"], skimDir)
    groupDir = "/nfs/dust/cms/group/susy-desy/david/MergeSkim/"

    toMerge = {}
    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    for d in os.listdir(absSkimDir):
        ##Skip if data and not nominal
        if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "":
            continue

        ##Skip Down for nominal case
        if syst == "Nominal" and shift == "Down":
            continue

        ##Get name and remove extension
        name = d[:-5] if "ext" in d else d
    
        if not name in toMerge:
            toMerge[name] = []

        ##Open file with xrootd paths to output files
        with open("{}/{}/outputFiles.txt".format(absSkimDir, d)) as fileList:
            for f in fileList:
                fileName = ""

                if syst != "":
                    if systName in f:
                        fileName = f.replace("\n", "")

                else:
                    if "Up" not in f and "Down" not in f:
                        fileName = f.replace("\n", "")

                if(fileName != ""):
                    toMerge[name].append(fileName)

    for name, files in toMerge.items():
        chunkSize = 20
        tmpTask = []
        tmpFiles, dependencies = files, len(files)*[""]

        while(len(tmpFiles) > chunkSize):
            tmpDir = "/{}/{}/tmp/{}/{}/".format(skimDir, name, systName, len(tmpTask))

            task = {
                "name": "MergeSkim_{}_{}_{}_{}_{}_{}".format(channel, era, name, systName, len(tmpTask), postFix),
                "dir": "{}/{}/tmp/{}/{}".format(config["dir"].format_map(dd(str, {"C": channel, "E": era})), name, systName, len(tmpTask)),
                "executable": "merge",
                "run-mode": config["run-mode"],
                "dependencies": [dep for dep in dependencies[:chunkSize] if dep != ""],
                "arguments": {
                    "input-files": tmpFiles[:chunkSize],                  
                    "out-file": "{}/{}/{}.root".format(groupDir, tmpDir, name),
                    "exclude-objects": ["Lumi", "pileUp", "pileUpUp", "pileUpDown", "xSec"],
                }
            }

            if len(task["dependencies"]) == 0: 
                task["arguments"]["optmize"] = ""

            beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
            afterExe = [sendToDCache(task["arguments"]["out-file"], tmpDir.replace("Skim", "Tmp"))]

            tmpTask.append(Task(task, "--", beforeExe, afterExe))

            tmpFiles = tmpFiles[chunkSize:] + [task["arguments"]["out-file"]]
            dependencies = dependencies[chunkSize:] + [task["name"]]

        mergedDir = "/{}/{}/merged/{}/".format(skimDir, name, systName)

        task = {
            "name": "MergeSkim_{}_{}_{}_{}_{}".format(channel, era, name, systName, postFix),
            "dir": "{}/{}/merged/{}".format(config["dir"].format_map(dd(str, {"C": channel, "E": era})), name, systName),
            "executable": "merge",
            "run-mode": config["run-mode"],
            "dependencies": [dep for dep in dependencies if dep != ""],
            "arguments": {
                "input-files": tmpFiles,
                "out-file": "{}/{}/{}.root".format(os.environ["CHDIR"], mergedDir, name),
                "exclude-objects": ["Lumi", "pileUp", "pileUpUp", "pileUpDown", "xSec"],
            }
        }

        beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
        afterExe = [sendToDCache(task["arguments"]["out-file"], mergedDir)]

        tasks.extend(tmpTask)
        tasks.append(Task(task, "--", beforeExe, afterExe))

def mergeNanoSkim(tasks, config, channel, era, syst, shift, postFix = ""):
    dir = config["dir"].format_map(dd(str, {"C": channel, "E": era}))
    skimDir = config["skim-dir"].format_map(dd(str, {"C": channel, "E": era}))
    dCacheDir = "{}/{}".format("/pnfs/desy.de/cms/tier2/store/user/dbrunner/", skimDir)
 #   groupDir = "{}/{}".format("/nfs/dust/cms/group/susy-desy/david/MergeSkim/", skimDir)

    toMerge = {}
    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    for d in os.listdir(dCacheDir):
        ##Skip if data and not nominal
        if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "Nominal":
            continue

        ##Skip Down for nominal case
        if syst == "Nominal" and shift == "Down":
            continue

        ##Get name and remove extension
        name = d[:-5] if "ext" in d else d
    
        if not name in toMerge:
            toMerge[name] = []

        ##Open file with xrootd paths to output files
        for f in glob.glob("{}/{}/{}/unmerged/*/*.root".format(dCacheDir, name, systName)):
            toMerge[name].append(f)

    for name, files in toMerge.items():
        chunkSize = 30
        tmpTask = []
        tmpFiles, dependencies = files, len(files)*[""]

        while(len(tmpFiles) > chunkSize):
            tmpDir = "{}/{}/{}/{}/tmp/{}".format(os.environ["CHDIR"], skimDir, name, systName, len(tmpTask))

            task = {
                "name": "MergeSkim_{}_{}_{}_{}_{}_{}".format(channel, era, name, systName, len(tmpTask), postFix),
                "dir": "{}/{}/{}/tmp/{}".format(dir, name, systName, len(tmpTask)),
                "executable": "merge",
                "run-mode": config["run-mode"],
                "dependencies": [dep for dep in dependencies[:chunkSize] if dep != ""],
                "arguments": {
                    "input-files": tmpFiles[:chunkSize],                  
                    "out-file": "{}/{}.root".format(tmpDir, name),
                    "exclude-objects": config["exclude"],
                }
            }

            if len(task["dependencies"]) == 0: 
                task["arguments"]["optmize"] = ""

            beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
            afterExe = [sendToDCache(task["arguments"]["out-file"], tmpDir.replace(os.environ["CHDIR"], ""))]

            tmpTask.append(Task(task, "--", beforeExe, afterExe))

            tmpFiles = tmpFiles[chunkSize:] + [task["arguments"]["out-file"]]
            dependencies = dependencies[chunkSize:] + [task["name"]]

        mergedDir = "{}/{}/{}/{}/merged".format(os.environ["CHDIR"], skimDir, name, systName)

        task = {
            "name": "MergeSkim_{}_{}_{}_{}_{}".format(channel, era, name, systName, postFix),
            "dir": "{}/{}/{}/merged/".format(dir, name, systName),
            "executable": "merge",
            "run-mode": config["run-mode"],
            "dependencies": [dep for dep in dependencies if dep != ""],
            "arguments": {
                "input-files": tmpFiles,
                "out-file": "{}/{}.root".format(mergedDir, name),
                "exclude-objects": config["exclude"],
            }
        }

        if len(task["dependencies"]) == 0: 
            task["arguments"]["optmize"] = ""

        beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
        afterExe = [sendToDCache(task["arguments"]["out-file"], mergedDir.replace(os.environ["CHDIR"], ""))]

        tasks.extend(tmpTask)
        tasks.append(Task(task, "--", beforeExe, afterExe))

def DNN(tasks, config, channel, era, evType, postFix = ""):
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))

    skimBaseDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"])

    systName = "Nominal"

    indexPath = "{}/{}".format(os.environ["CHDIR"], config["index-path"])
    outDir = config["dir"].format_map(dd(str, {"D": "Network", "C": channel, "E": era, "R": evType}))

    classes, classesIndex, signals, signalsIndex = {}, {}, [], []

    for proc in os.listdir(skimBaseDir):
        for process in config["classes"] + config.get("Misc", []):
            for processName in processes[process]:
                if proc.startswith(processName) and not "_ext" in proc:
                    cls = process if process in config["classes"] else "Misc"
                          
                    classes.setdefault(cls, []).append("{}/merged/{}.root".format(skimDir.format_map(dd(str, {"C": channel, "E": era, "P": proc, "S": systName})), proc))
                    classesIndex.setdefault(cls, []).append("{}/merged/{}.csv".format(indexPath.format_map(dd(str, {"C": channel, "E": era, "P": proc, "S": systName, "R": evType})), proc))

        for process in config["signal"]:
            for processName in processes[process]:
                if proc.startswith(processName):
                    signals.append("{}/merged/{}.root".format(skimDir.format_map(dd(str, {"C": channel, "E": era, "P": proc, "S": systName})), proc))
                    signalsIndex.append("{}/merged/{}.csv".format(indexPath.format_map(dd(str, {"C": channel, "E": era, "P": proc, "S": systName, "R": evType})), proc))

    os.makedirs(outDir, exist_ok=True)

    with open(outDir + "/parameter.csv", "w") as param:
        param.write("Parameter\n")

        for parameter in config["parameters"].get("all", []) + config["parameters"].get(channel, []):
            param.write(parameter + "\n")
                
    with open(outDir + "/classes.csv", "w") as c:
        c.write("ClassName\n")

        for cls in classes.keys():
            c.write(cls + "\n")
                
    with open(outDir + "/masses.csv", "w") as masses:
        masses.write("ChargedMass\tNeutralMass\n")

        for sig in config["signal"]:
            mHC, mh = sig.replace("HPlus", "").replace("h", "").replace("_4B", "").split("_")
            masses.write("{}\t{}\n".format(mHC, mh))

    task = {
        "name": "DNN_{}_{}_{}_{}".format(channel, era, evType, postFix),
        "executable": "dnn",
        "dir": outDir,
        "run-mode": config["run-mode"],
        "arguments": {
            "out-path": outDir,
            "channel": channel,   
            "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
            "sig-files": signals,
            "sig-entry-list": signalsIndex,
            "opt-param": config.get("hyper-opt", "").format_map(dd(str, {"C": channel, "E": era, "T": evType})),
            "era": era,
            "bkg-classes": list(classes.keys()),
        }
    }

    if task["arguments"]["opt-param"] != "":
        task["arguments"]["opt-param"] = os.environ["CHDIR"] + "/" + task["arguments"]["opt-param"]

    for cls, files in classes.items():
        task["arguments"]["{}-files".format(cls)] = files

    for cls, files in classesIndex.items():
        task["arguments"]["{}-entry-list".format(cls)] = files

    tasks.append(Task(task, "--"))

def eventCount(tasks, config, channel, era, processes, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    outDir = config["dir"].format_map(dd(str, {"D": "EventCount", "C": channel, "E": era})) + "/" + systName
    files, dependencies = [], []

    for process in processes:
       for t in tasks:
            if "MergeHists_{}_{}_{}_".format(channel, era, process) in t["name"] and postFix in t["name"]:
                if systName in t["name"] and systName != "Nominal":
                    dependencies.append(t["name"])
                    files.append(t["arguments"]["out-file"])

                elif syst == "Nominal" and not "Up" in t["name"] and not "Down" in t["name"]:
                    dependencies.append(t["name"])
                    files.append(t["arguments"]["out-file"])

                elif "Single" in t["name"]:
                    dependencies.append(t["name"])
                    files.append(t["arguments"]["out-file"])

            elif process == "QCD" and "QCDEstimate_{}_{}".format(channel, era) in t["name"] and postFix in t["name"]:
                dependencies.append(t["name"])
                files.append(t["arguments"]["out-file"])
                    
    task = {
        "name": "EventCount_{}_{}_{}_{}".format(channel, era, systName, postFix), 
        "executable": "eventcount",
        "dir": outDir,
        "dependencies": dependencies,
        "arguments": {
            "processes": processes,
            "files": files,
            "out-dir": outDir,
        }
    }

    tasks.append(Task(task, "--"))


def datacard(tasks, config, channel, era, processes, mHPlus, mH, postFix = ""):
    dependencies = []
    bkgFiles, sigFiles, dataFile = {}, {}, {}

    outDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": channel, "E": era}))
    systematics = config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []) + config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, [])
    regions = ["SR", "VR"] + config.get("control-regions", [])

    for t in tasks:
        for region in regions:
            if "MergeHists_{}_{}".format(channel, era) in t["name"] and "{}_{}".format(mHPlus, mH) in t["name"] and "_{}Region".format(region) in t["name"]:
                dependencies.append(t["name"])

            else:
                continue

            for syst in systematics:
                for shift in ["Up", "Down"]:
                    if syst == "Nominal" and shift == "Down":
                        continue

                    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
                    systCheck = systName in t["name"] if syst != "" else not "Up" in t["name"] and not "Down" in t["name"]

                    for proc in config["backgrounds"]:
                        if "_{}_".format(proc) in t["name"] and systCheck:
                            bkgFiles.setdefault(region, {}).setdefault(systName, []).append(t["arguments"]["out-file"])
                            break

                    if config["signal"] in t["name"] and systCheck:
                        sigFiles.setdefault(region, {}).setdefault(systName, []).append(t["arguments"]["out-file"])

            if config.get("data", {}).get(channel, []) and config["data"].get(channel, [])[0] in t["name"]:
                dataFile[region] = t["arguments"]["out-file"]
                   
    task = {
        "name": "DataCard_{}_{}_{}_{}_{}".format(channel, era, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "writecard",
        "dependencies": dependencies,
        "arguments": {
            "discriminant": config["discriminant"],
            "bkg-processes": config["backgrounds"],
            "sig-process": config["signal"],
            "data": config["data"].get(channel, [""])[0],
            "out-dir": outDir,
            "channel": channel,
            "era": era,
            "systematics": ['""'] + systematics[1:],
            "region-names": regions,
        }
    }

    for region in regions:
        task["arguments"]["data-file-{}".format(region)] = dataFile[region]

        for syst in systematics:
            for shift in ["Up", "Down"]:
                if syst == "Nominal" and shift == "Down":
                    continue

                systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

                task["arguments"]["bkg-files-{}".format(region) + ("-{}".format(systName) if systName != "Nominal" else "")] = bkgFiles[region][systName]
                task["arguments"]["sig-files-{}".format(region) + ("-{}".format(systName) if systName != "Nominal" else "")] = sigFiles[region][systName]

    tasks.append(Task(task, "--"))

def limits(tasks, config, channel, era, mHPlus, mH, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "Limit", "C": channel, "E": era}))
    cardDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": channel, "E": era}))
    data = config.get("data", {}).get(channel, [])

    preExeCommands = [
        "source $CHDIR/ChargedAnalysis/setenv.sh CMSSW",
        "cd {}".format(outDir),
        "text2workspace.py {dir}/datacard.txt -o workspace.root".format(dir = cardDir)
    ]

    postExeCommands = [
        "mv higgs*.root limit.root",
        "combine -M FitDiagnostics -d workspace.root --saveShapes --saveWithUncertainties --rMin -200 --rMax 200",
        "rm higgs*.root"
    ]

    task = {
        "name": "Limit_{}_{}_{}_{}_{}".format(channel, era, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "combine",
        "dependencies": [t["name"] for t in tasks if t["dir"] == cardDir],
        "arguments": {
            "method": "AsymptoticLimits",
            "datacard": "workspace.root",
            "saveWorkspace": ""
        }
    }

    tasks.append(Task(task, "--", preExeCommands, postExeCommands))

def plotlimit(tasks, config, channel, era, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "LimitPDF", "C": channel, "E": era}))
    webDir = ""

    if "Results/Limit" in outDir:
        webDir = outDir.replace("Results", "WebPlotting/Plots").replace("LimitPDF", "")

    xSecs, limitFiles, dependencies = [], [], []

    for i, t in enumerate(tasks):
        for mHPlus in config["charged-masses"]:
            for mH in config["neutral-masses"]:
                if i == 0:
                    f = ROOT.TFile.Open("/pnfs/desy.de/cms/tier2//store/user/dbrunner/Skim/19_03_2021/Inclusive/2016/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}/merged/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}.root".format(MHC=mHPlus, MH=mH))
                    xSecs.append(f.Get("xSec").GetVal())

                if "Limit_{}_{}_{}_{}".format(channel, era, mHPlus, mH) in t["name"]:
                    dependencies.append(t["name"])
                    limitFiles.append("{}/limit.root".format(t["dir"]))

    task = {
        "name": "PlotLimit_{}_{}_{}".format(channel, era, postFix), 
        "dir": outDir,
        "executable": "plotlimit",
        "dependencies": dependencies,
        "arguments": {
            "limit-files": limitFiles,
            "charged-masses": config["charged-masses"],
            "neutral-masses": config["neutral-masses"],
            "channel": channel,
            "era": era,
            "x-secs": xSecs,
            "out-dirs": [outDir, webDir] if webDir != "" else [outDir],
        }
    }

    tasks.append(Task(task, "--"))

def plotimpact(tasks, config, channel, era, mHPlus, mH, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "ImpactsPDF", "C": channel, "E": era}))
    webDir = ""
    data = config.get("data", {}).get(channel, [])

    if "Results/Limit" in outDir:
        webDir = outDir.replace("Results", "WebPlotting/Plots").replace("ImpactsPDF", "")

    dependencies = []

    for t in tasks:
        if "Limit_{}_{}_{}_{}".format(channel, era, mHPlus, mH) in t["name"]:
            dependencies.append(t["name"])
            inputFile = "{}/workspace.root".format(t["dir"])
            break

    preExeCommands = [
        "source $CHDIR/ChargedAnalysis/setenv.sh CMSSW",
        "cd {}".format(outDir),
        "combineTool.py -M Impacts -d {} -m 100 --doInitialFit --robustFit 1 --rMin -200 --rMax 200".format(inputFile),
        "combineTool.py -M Impacts -d {} -m 100 --robustFit 1 --doFits --rMin -200 --rMax 200".format(inputFile),
        "combineTool.py -M Impacts -d {} -m 100 -o impacts.json --rMin -200 --rMax 200".format(inputFile),
    ]

    task = {
        "name": "PlotImpacts_{}_{}_{}_{}_{}".format(channel, era, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "plotImpacts.py",
        "dependencies": dependencies,
        "arguments": {
            "input": "impacts.json",
            "output": "impacts",
        }
    }

    tasks.append(Task(task, "--", preExeCommands))

def plotpostfit(tasks, config, channel, era, mHPlus, mH, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "PostfitPDF", "C": channel, "E": era}))
    webDir = ""

    if "Results/Limit" in outDir:
        webDir = outDir.replace("Results", "WebPlotting/Plots").replace("PostfitPDF", "")

    inputFile, dependencies = "", []

    for t in tasks:
        if "Limit_{}_{}_{}_{}".format(channel, era, mHPlus, mH) in t["name"]:
            dependencies.append(t["name"])
            inputFile = "{}/fitDiagnostics.root".format(t["dir"])

    task = {
        "name": "PlotPostFit_{}_{}_{}_{}_{}".format(channel, era, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "plotpostfit",
        "dependencies": dependencies,
        "arguments": {
            "in-file": inputFile,
            "bkg-processes": config["backgrounds"],
            "sig-process": config["signal"],
            "out-dirs": [outDir, webDir] if webDir != "" else [outDir],
        }
    }

    tasks.append(Task(task, "--"))

def mergeDataCards(tasks, config, channels, eras, mHPlus, mH, postFix = ""):
    chanName, chanList = channels.popitem() if type(channels) == dict else (channels, [])
    eraName, eraList = eras.popitem() if type(eras) == dict else (eras, [])

    outDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": chanName, "E": eraName}))
    dependencies = []
    args = ""

    for t in tasks:
        for chan in chanList if len(chanList) != 0 else [chanName]:
            for era in eraList if len(eraList) != 0 else [eraName]:
                if "DataCard_{}_{}_{}_{}".format(chan, era, mHPlus, mH) in t["name"]:
                    dependencies.append(t["name"])
                    args += "{}/datacard.txt ".format(t["dir"])

    task = {
        "name": "MergeCards_{}_{}_{}_{}_{}".format(chanName, eraName, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "combineCards.py",
        "dependencies": dependencies,
        "arguments": {
            args + "> {}/datacard.txt".format(outDir) : "",
        }
    }

    tasks.append(Task(task, "", ["source $CHDIR/ChargedAnalysis/setenv.sh CMSSW"]))

def nanoSkim(tasks, config, era):
    CMSSWpath = "{}/{}".format(os.environ["CHDIR"], config["CMSSW"])
    filePath = "{}/src/ChargedSkimming/Skimming/data/filelists/{}/UL".format(CMSSWpath, era)

    date = "_".join([str(getattr(time.localtime(), "tm_" + t)) for t in ["mday", "mon", "year"]])
    xSecFile = yaml.load(open("{}/src/ChargedSkimming/Skimming/data/xsec.yaml".format(CMSSWpath), "r"), Loader=yaml.Loader)

    names = []

    for proc in ["sig", "bkg", "data"]:
         with open("{}/filelist_{}_NANO.yaml".format(filePath, proc)) as f:
            datasets = yaml.load(f, Loader=yaml.Loader)

            for dataset in datasets:
                if "SIM" in dataset or "HPlus" in dataset:
                    name = dataset.split("/")[1]

                    nExt = len([n for n in names if name in n])

                    if nExt != 0:
                        name += "_ext{}".format(nExt)
                    
                else: 
                    name = dataset.split("/")[1] + "_" + dataset.split("/")[2]

                names.append(name)

                xSec = 1.

                for key in xSecFile.keys():
                    if key in name:
                        xSec = xSecFile[key]["xSec"]

                run = name[name.find("Run") + 7] if "Run" in name else "MC"

                fileList = subprocess.check_output("source $CHDIR/ChargedAnalysis/setenv.sh CMSSW; dasgoclient -query='file dataset={}'".format(dataset), shell = True).decode().split("\n")

                fileList = ["root://cms-xrd-global.cern.ch/{}".format(f) for f in fileList if "NANO" in f]

                for idx, fName in enumerate(fileList):
                    tdir = "/tmp/NanoSkim_{}_{}_{}".format(name, era, idx)
                    beforeExe = ["mkdir -p {}".format(tdir), "cd {}".format(tdir), "source $CHDIR/ChargedAnalysis/setenv.sh CMSSW", "while [ ! -f input.root ]; do xrdcp -f {} input.root; done".format(fName)]

                    task = {
                        "name": "NanoSkim_{}_{}_{}".format(name, era, idx), 
                        "dir": "{}/{}/unmerged/{}".format(config["dir"].format(E = era), name, idx),
                        "executable": "NanoSkim",
                        "dependencies": [],
                        "run-mode": config["run-mode"],
                        "arguments": {
                            "file-name": "input.root",
                            "xSec": xSec,
                            "run": run,
                            "era": era,
                            "out-file": "{}.root".format(name),
                            "out-dir": "Skim/{}/[C]/{}/{}/[SYS]/unmerged/{}".format(date, era, name, idx),
                            "channels": "'{}'".format(" ".join(config["channels"])),
                            "systematics": "'{}'".format(" ".join(config["systematics"])) if run == "MC" else "Nominal",                    
                        }
                    }

                    afterExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis", "for D in $(echo {}); do gfal-mkdir -p $SRM_DESY/dbrunner/$D; gfal-copy -f $D/{}.root $SRM_DESY/dbrunner/$D/{}.root; done".format(task["arguments"]["out-dir"].replace("[C]", "*").replace("[SYS]", "*"), name, name)]

                    tasks.append(Task(task, "--", beforeExe = beforeExe, afterExe = afterExe))
