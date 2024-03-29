from task import Task

import yaml
import os
import copy
import csv
import time
import subprocess
import glob
import numpy as np
from pprint import pprint

import ROOT

from collections import defaultdict as dd

def sendToDCache(inFile, relativPath, timeOut = 1800):
    dCachePath = "/pnfs/desy.de/cms/tier2/store/user/dbrunner/"

    command = "sendToDCache \\\n{}--input-file \\\n{}{} \\\n{}--dcache-path \\\n{}{} \\\n{}--relative-path \\\n{}{} \\\n{}--time-out \\\n{}{}\n".format(2*" ", 6*" ", inFile, 2*" ", 6*" ", dCachePath, 2*" ", 6*" ", relativPath, 2*" ", 6*" ", timeOut)

    return command

def treeread(tasks, config, channel, era, process, syst, shift, mHPlus = "200", postFix = ""):
    ##Mis.id. jets are configured together with data process, so skip here
    if process == "MisIDJ":
        return

    ##Dic with process:filenames 
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "Nominal" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != "Nominal"): 
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    skimBaseDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"])
        
    ## Scale systematics
    scaleSysts = []

    if syst == "Nominal" and process not in ["SingleE", "SingleMu", "MisIDJ"]:
        scaleSysts = config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, [])

    fileNames = []

    ##Data driven scale factors           
    bkgYldFactor, bkgYldFactorSyst, bkgType = "", [], "Misc"

    if "bkg-yield-factor" in config:
        if process not in ["SingleE", "SingleMu"] and "HPlus" not in process:
            bkgYldFactor = "{}/{}".format(os.environ["CHDIR"], config["bkg-yield-factor"].format_map(dd(str, {"C": channel, "E": era, "MHC": mHPlus, "S": systName})))

            with open(bkgYldFactor) as f:
                reader = csv.reader(f, delimiter='\t')

                for row in reader:
                    if process in row:
                        bkgType = process
                        break

            for scaleSyst in scaleSysts:
                for scaleShift in ["Up", "Down"]:
                    bkgYldFactorSyst.append("{}/{}".format(os.environ["CHDIR"], config["bkg-yield-factor"].format_map(dd(str, {"C": channel, "E": era, "MHC": mHPlus, "S": systName}))))

    #List of parameters
    parameters = config["parameters"].get("all", []) + config["parameters"].get(channel, [])

    ##List of filenames for each process
    for proc in os.listdir(skimBaseDir):
        for processName in processes[process]:
            if proc.startswith(processName) and not "_ext" in proc:
                fileNames.append("{}/merged/{}.root".format(skimDir.format_map(dd(str, {"C": channel, "E": era, "P": proc, "S": systName})), proc))

    nJobs = 0

    for fileName in fileNames:
        try:
            f = ROOT.TFile.Open(fileName)
            t = f.Get(channel)   
            nEvents = t.GetEntries()
            f.Close()

        except:
            print("Could not read file, which is skipped: {}".format(fileName))
            continue

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
                    "out-file": "{}.root".format(process),
                    "channel": channel,
                    "regions": [],
                    "scale-systs": scaleSysts,
                    "bkg-yield-factor": bkgYldFactor,
                    "bkg-yield-factor-syst": bkgYldFactorSyst,
                    "bkg-type": bkgType,
                    "era": era,
                    "event-start": start,
                    "event-end": end
                }
            }

            if process in ["SingleE", "SingleMu"] and "MisIDJ" in config["processes"]:
                task["arguments"]["fake-rate"] = os.environ["CHDIR"] + "/" + config["fake-estimate"]["rates"].format_map(dd(str, {"C": "EleIncl" if "Ele" in channel else "MuonIncl", "E": era, "R": "fake", "S": systName}))
                task["arguments"]["prompt-rate"] = os.environ["CHDIR"] + "/" + config["fake-estimate"]["rates"].format_map(dd(str, {"C": "EleIncl" if "Ele" in channel else "MuonIncl", "E": era, "R": "prompt", "S": systName}))

            for region in config.get("regions", ["Default"]):
                task["arguments"]["{}-out-dir".format(region)] = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": region, "P": process, "S": systName})) + "/unmerged/{}".format(nJobs)
                task["arguments"]["{}-syst-dirs".format(region)] = [task["arguments"]["{}-out-dir".format(region)].replace(systName, "{}{}".format(scaleSyst, scaleShift)) for scaleSyst in scaleSysts for scaleShift in ["Up", "Down"]]
                task["arguments"]["{}-cuts".format(region)] = config["cuts"].get("all", []) + config["cuts"].get("allRegions", {}).get(region, [])
                task["arguments"]["regions"].append(region)

                if channel in config["cuts"]:
                    if region in config["cuts"][channel]:
                        task["arguments"]["{}-cuts".format(region)].extend(config["cuts"][channel][region])

                    if "all" in config["cuts"][channel]:
                        task["arguments"]["{}-cuts".format(region)].extend(config["cuts"][channel]["all"])

                    if not region in config["cuts"][channel] and not "all" in config["cuts"][channel]:
                        task["arguments"]["{}-cuts".format(region)].extend(config["cuts"][channel])

                if process in ["SingleE", "SingleMu"] and "MisIDJ" in config["processes"]:
                    lRegion = "Loose-{}".format(region)

                    task["arguments"]["{}-out-dir".format(lRegion)] = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era, "R": lRegion, "P": process, "S": systName})) + "/unmerged/{}".format(nJobs)
                    task["arguments"]["{}-syst-dirs".format(lRegion)] = []
                    task["arguments"]["{}-cuts".format(lRegion)] = [c for c in task["arguments"]["{}-cuts".format(region)] if not "replaceForFR" in c]
                    task["arguments"]["{}-cuts".format(lRegion)].extend(config["fake-estimate"]["cuts"][channel])
                    task["arguments"]["regions"].append(lRegion)

            tasks.append(Task(task, "--"))

            nJobs += 1

def mergeHists(tasks, config, channel, era, process, syst, shift, postFix = ""):
    inputFiles, dependencies = {}, []

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "Nominal" and shift == "Down") or (process in ["SingleE", "SingleMu", "MisIDJ"] and syst != "Nominal"): 
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    pName = process if process != "MisIDJ" else "Single"

    for t in tasks:
        if "TreeRead_{}_{}_{}".format(channel, era, pName) in t["name"] and postFix in t["name"]:
            if systName in t["name"]:
                dependencies.append(t["name"])

                for region in config.get("regions", ["Default"]):
                    if process != "MisIDJ":
                        inputFiles.setdefault(region, []).append("{}/{}".format(t["arguments"]["{}-out-dir".format(region)], t["arguments"]["out-file"]))

                    else:
                        inputFiles.setdefault(region, []).append("{}/{}".format(t["arguments"]["{}-out-dir".format(region)], t["arguments"]["out-file"]).replace("SingleMu", "MisIDJ").replace("SingleE", "MisIDJ"))
                                 
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
                task["arguments"]["{}-cuts".format(region)] = config["cuts"].get("all", [])

                if channel in config["cuts"]:
                    if region in config["cuts"][channel]:
                        task["arguments"]["{}-cuts".format(region)] += config["cuts"][channel][region]

                    elif "all" in config["cuts"][channel]:
                        task["arguments"]["{}-cuts".format(region)] += config["cuts"][channel][region]

                    else:
                        task["arguments"]["{}-cuts".format(region)] += config["cuts"][channel]

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

                        elif process == "MisIDJ":
                            if syst == "Nominal":
                                bkgProcesses.setdefault(region, []).append(process)
                            bkgFiles.setdefault(region, {}).setdefault(systName, []).append(t["arguments"]["out-file"])

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
        if "Run2" in fileName and syst != "Nominal":
            continue

        ##Ignore extensions
        if "_ext" in fileName:
            continue

        nJobs = 0

        skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era, "P": fileName, "S": systName})))

        try:
            f = ROOT.TFile.Open("{}/merged/{}.root".format(skimDir, fileName))
            t = f.Get(channel)   
            nEvents = t.GetEntries()
            f.Close()

        except:
            raise RuntimeError("Problem with file: {}/{}".format(skimDir, fileName))

        eventRanges = [[i if i == 0 else i+1, i+config["n-events"] if i+config["n-events"] <= nEvents else nEvents] for i in range(0, nEvents, config["n-events"])]

        ##Configuration for treeread Task
        for start, end in eventRanges:
            d = "{}/unmerged/{}".format(config["dir"].format_map(dd(str, {"C": channel, "E": era, "P": fileName, "S": systName})), nJobs)

            ##Configuration for treeread Task
            task = {
                "name": "Append_{}_{}_{}_{}_{}_{}".format(channel, era, fileName, systName, nJobs, postFix),
                "dir":  d,
                "executable": "treeappend",
                "run-mode": config["run-mode"],
                "arguments": {
                    "in-file": "{}/merged/{}.root".format(skimDir, fileName),
                    "out-file": "{}/{}.root".format(d, fileName),
                    "tree-name": channel,
                    "functions": list(config["functions"].keys()),
                    "era": era,
                    "event-start": start,
                    "event-end": end
                }
            }

            for function in task["arguments"]["functions"]:
                for info in config["functions"][function]:
                    task["arguments"]["{}-{}".format(function, info)] = config["functions"][function][info]

            beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
            afterExe = [sendToDCache(task["arguments"]["out-file"], d.replace(os.environ["CHDIR"], ""))]

            nJobs += 1

            tasks.append(Task(task, "--", beforeExe, afterExe))

def mergeAppend(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    ##Stuff to exclude
    toExclude = []

    f = ROOT.TFile("{}/{}/TTToSemiLeptonic_TuneCP5_13TeV-powheg-pythia8/{}/merged/TTToSemiLeptonic_TuneCP5_13TeV-powheg-pythia8.root".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})), systName))

    for item in f.GetListOfKeys():
        if(item.GetName() != channel):
            toExclude.append(item.GetName())

    ##Get dependent tasks
    skimBaseDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))

    toMerge = {}
    dependencies = {}

    processes = [p for p in os.listdir(skimBaseDir)]

    for t in tasks:
        for p in processes:
            ##Skip if data and not nominal
            if ("Electron" in p or "Muon" in p or "Gamma" in p) and syst != "Nominal":
                continue

            ##Ignore extensions
            if "_ext" in p:
                continue

            if "Append_{}_{}_{}_{}".format(channel, era, p, systName) in t["name"] and postFix in t["name"]:
                toMerge.setdefault(p, []).append(t["arguments"]["out-file"])
                dependencies.setdefault(p, []).append(t["name"])

    for name, files in toMerge.items():
        chunkSize = 30
        tmpTask = []
        tmpFiles, deps, mergeDependencies = files, dependencies[name], len(files)*[""]

        mergedDir = "{}/merged".format(config["dir"].format_map(dd(str, {"C": channel, "E": era, "P": name, "S": systName})))
        skimDir = config["skim-dir"].format_map(dd(str, {"C": channel, "E": era, "P": name, "S": systName}))

        while(len(tmpFiles) > chunkSize):
            tmpDir = mergedDir.replace("merged", "tmp/{}".format(len(tmpTask)))

            task = {
                "name": "MergeAppend_{}_{}_{}_{}_{}_{}".format(channel, era, name, systName, len(tmpTask), postFix),
                "dir": tmpDir,
                "executable": "merge",
                "run-mode": config["run-mode"],
                "dependencies": [dep for dep in mergeDependencies[:chunkSize] if dep != ""] + [dep for dep in deps[:chunkSize] if dep != ""],
                "arguments": {
                    "input-files": tmpFiles[:chunkSize],                  
                    "out-file": "{}/{}.root".format(tmpDir, name),
                    "exclude-objects": toExclude,
                }
            }

            if len(task["dependencies"]) == 0: 
                task["arguments"]["optmize"] = ""

            beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
            afterExe = [sendToDCache(task["arguments"]["out-file"], tmpDir.replace(os.environ["CHDIR"], ""))]

            tmpTask.append(Task(task, "--", beforeExe, afterExe))

            deps = deps[chunkSize:] + [""]
            tmpFiles = tmpFiles[chunkSize:] + [task["arguments"]["out-file"]]
            mergeDependencies = mergeDependencies[chunkSize:] + [task["name"]]

        task = {
            "name": "MergeSkim_{}_{}_{}_{}_{}".format(channel, era, name, systName, postFix),
            "dir": mergedDir,
            "executable": "merge",
            "run-mode": config["run-mode"],
            "dependencies": [dep for dep in mergeDependencies if dep != ""] + [dep for dep in deps[:chunkSize] if dep != ""],
            "arguments": {
                "input-files": tmpFiles,
                "out-file": "{}/{}/merged/{}.root".format(os.environ["CHDIR"], skimDir, name),
                "exclude-objects": toExclude,
            }
        }

        if len(task["dependencies"]) == 0: 
            task["arguments"]["optmize"] = ""

        beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
        afterExe = [sendToDCache(task["arguments"]["out-file"], skimDir)]

        tasks.extend(tmpTask)
        tasks.append(Task(task, "--", beforeExe, afterExe))

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


def promptrate(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    outDir = config["dir"].format_map(dd(str, {"D": "Rates", "C": channel, "E": era, "R": "prompt", "S": systName}))
    data = config["data"].get(channel, ["data"])[0]
    process = config["proc-usage"]["use"]["prompt"]

    depFiles, depTasks = {}, []

    for t in tasks:
        if "MergeHists_{}_{}_".format(channel, era) in t["name"] and ("prompt-loose" in t["name"] or "prompt-tight" in t["name"]):
            region = "prompt-loose" if "prompt-loose" in t["name"] else "prompt-tight"

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
        "name": "Promptrate_{}_{}_{}_{}".format(channel, era, systName, postFix),
        "dir": outDir,
        "executable": "estimatePR",
        "dependencies": [t["name"] for t in depTasks],
        "arguments": {
            "out-dir": outDir,
            "hist-name": config["hist-name"]["prompt"][channel],
            "process":  config["proc-usage"]["use"]["prompt"],
        }
    }

    task["arguments"]["{}-{}-file".format("prompt-loose", process)] = depFiles["prompt-loose"][process]
    task["arguments"]["{}-{}-file".format("prompt-tight", process)] = depFiles["prompt-tight"][process]

    plotTask = {
        "name": "PlotPromptrate_{}_{}_{}_{}".format(channel, era, systName, postFix),
        "dir": outDir.replace("Rates", "ResultPDF"),
        "executable": "plotFR",
        "dependencies": [task["name"]],
        "arguments": {
            "mode": "prompt",
            "out-dir": [outDir.replace("Rates", "ResultPDF"), outDir.replace("Results", "WebPlotting/Plots").replace("Rates", "")],
            "era": era,
            "channel": channel,
            "input-file": "{}/promptrate.root".format(outDir)
        }
    }
            
    tasks.append(Task(task, "--"))
    tasks.append(Task(plotTask, "--"))

def fakerate(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    outDir = config["dir"].format_map(dd(str, {"D": "Rates", "C": channel, "E": era, "R": "fake", "S": systName}))
    data = config["data"].get(channel, ["data"])[0]

    bins = []

    for ptLow, ptHigh in config["fake-bins"]["pt"]["bins"]:
        for etaLow, etaHigh in config["fake-bins"]["eta"]["bins"]:
            bins.append("{}_{}_{}_{}".format(ptLow, ptHigh, etaLow, etaHigh))

    depFiles, depTasks = {}, []

    for t in tasks:
        if "MergeHists_{}_{}_".format(channel, era) in t["name"] and "fake" in t["name"]:
            region = "fake-loose" if "fake-loose" in t["name"] else "fake-tight" if not "fake-no-lep" in t["name"] else "fake-no-lep"
            
            for b in bins:
                if b in t["name"]:
                    region += "_" + b
                    break

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
        "name": "Fakerate_{}_{}_{}_{}".format(channel, era, systName, postFix),
        "dir": outDir,
        "executable": "estimateFR",
        "dependencies": [t["name"] for t in depTasks],
        "arguments": {
            "out-dir": outDir,
            "web-dir": outDir.replace("Results", "WebPlotting/Plots").replace("Rates", "").replace("fake", "PostFit"),
            "bins": bins,
            "bkg-processes": config["processes"],
            "hist-name": config["hist-name"]["fake"][channel],
            "process":  config["proc-usage"]["use"]["fake"],
            "channel": channel,
            "era": era
        }
    }

    plotTask = {
        "name": "PlotFakerate_{}_{}_{}_{}".format(channel, era, systName, postFix),
        "dir": outDir.replace("Rates", "ResultPDF"),
        "executable": "plotFR",
        "dependencies": [task["name"]],
        "arguments": {
            "mode": "fake",
            "out-dir": [outDir.replace("Rates", "ResultPDF"), outDir.replace("Results", "WebPlotting/Plots").replace("Rates", "")],
            "era": era,
            "channel": channel,
            "input-file": "{}/fakerate.root".format(outDir)
        }
    }
         
    for b in bins:
        for process in [task["arguments"]["process"]] + task["arguments"]["bkg-processes"]:
            task["arguments"]["fake-loose-{}_{}-file".format(process, b)] = depFiles["fake-loose_{}".format(b)][process]
            task["arguments"]["fake-tight-{}_{}-file".format(process, b)] = depFiles["fake-tight_{}".format(b)][process]
            task["arguments"]["fake-no-lep-{}_{}-file".format(process, b)] = depFiles["fake-no-lep_{}".format(b)][process]
    
    tasks.append(Task(task, "--"))
    tasks.append(Task(plotTask, "--"))

def bkgEstimation(tasks, config, channel, era, syst, shift, mHPlus, mH, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"
    outDir = config["dir"].format_map(dd(str, {"D": "ScaleFactor", "C": channel, "E": era, "S": systName, "R": "{}-{}".format(mHPlus, mH)}))
    data = config["data"][channel][0]

    depTasks = []
    regions = []
    bkgFiles, dataFile = {}, {}

    for t in tasks:
        for region in config["regions"]:
            if "MergeHists_{}_{}_{}-{}-{}".format(channel, era, region, mHPlus, mH) in t["name"]:
                if systName in t["name"] and systName != "Nominal":
                    depTasks.append(t)
                    bkgFiles.setdefault(region, []).append(t["arguments"]["out-file"])

                elif data in t["name"]:
                    depTasks.append(t)
                    dataFile[region] = t["arguments"]["out-file"]

                elif syst == "Nominal" and not "Up" in t["name"] and not "Down" in t["name"]:
                    depTasks.append(t)
                    bkgFiles.setdefault(region, []).append(t["arguments"]["out-file"])

    task = {
        "name": "Estimate_{}_{}_{}_{}_{}_{}".format(channel, era, systName, mHPlus, mH, postFix),
        "dir": outDir,
        "executable": "estimate",
        "dependencies": [t["name"] for t in depTasks],
        "arguments": {
            "out-dir": outDir,
            "processes": config["regions"],
            "parameter": config["parameter"],
        }
    }
          
    for proc in config["regions"]:                        
        task["arguments"]["{}-bkg-files".format(proc)] = sorted(bkgFiles[proc], key = lambda s : s.split("/")[-1].split(".")[0] in config["regions"], reverse = True)
        task["arguments"]["{}-data-file".format(proc)] = dataFile[proc]

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
    webDir = outDir.replace("Results", "WebPlotting/Plots").replace("Network", "")

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
            "out-path": [outDir, webDir],
            "channel": channel,   
            "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
            "sig-files": signals,
            "sig-entry-list": signalsIndex,
            "opt-param": config.get("hyper-opt", "").format_map(dd(str, {"C": channel, "E": era})),
            "era": era,
            "bkg-classes": list(classes.keys()),
        }
    }

    if config.get("optimize", False):
        task["arguments"]["optimize"] = ""

    if task["arguments"]["opt-param"] != "":
        task["arguments"]["opt-param"] = os.environ["CHDIR"] + "/" + task["arguments"]["opt-param"]

    for cls, files in classes.items():
        task["arguments"]["{}-files".format(cls)] = files

    for cls, files in classesIndex.items():
        task["arguments"]["{}-entry-list".format(cls)] = files

    tasks.append(Task(task, "--"))

def mergeDNNOpt(tasks, config, channel, era, postFix = ""):
    dependencies, inputFiles = [], []

    for t in tasks:
        if "DNN_{}_{}".format(channel, era) in t["name"]:
            dependencies.append(t["name"])
            inputFiles.append("{}/hyperparam.csv".format(t["arguments"]["out-path"][0]))

    outDir = config["dir"].format_map(dd(str, {"D": "Network", "C": channel, "E": era, "R": "Even"}))

    task = {
        "name": "MergeOpt_{}_{}_{}".format(channel, era, postFix),
        "executable": "mergeOpt.py",
        "dir": outDir,
        "dependencies": dependencies,
        "run-mode": config["run-mode"],
        "arguments": {
            "input-files": inputFiles,
            "output": "{}/hyperparam.csv".format(outDir)
        }
    }

    tasks.append(Task(task, "--"))

def eventCount(tasks, config, channel, era, processes, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "Nominal" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "Nominal" else "Nominal"

    files, dependencies = {}, {}

    for t in tasks:
        for process in processes:
            for region in config.get("regions", ["Default"]):
                if "MergeHists_{}_{}_{}_{}".format(channel, era, region, process) in t["name"] and postFix in t["name"]:
                    if systName in t["name"] and systName != "Nominal":
                        dependencies.setdefault(region, []).append(t["name"])
                        files.setdefault(region, []).append(t["arguments"]["out-file"])

                    elif syst == "Nominal" and not "Up" in t["name"] and not "Down" in t["name"]:
                        dependencies.setdefault(region, []).append(t["name"])
                        files.setdefault(region, []).append(t["arguments"]["out-file"])

                    elif "Single" in t["name"] or "MisIDJ" in t["name"]:
                        dependencies.setdefault(region, []).append(t["name"])
                        files.setdefault(region, []).append(t["arguments"]["out-file"])
          
    for region in config.get("regions", ["Default"]):    
        outDir = config["dir"].format_map(dd(str, {"D": "Count", "C": channel, "E": era, "R": region, "S": systName}))
      
        task = {
            "name": "EventCount_{}_{}_{}_{}_{}".format(channel, era, region, systName, postFix), 
            "executable": "eventcount",
            "dir": outDir,
            "dependencies": dependencies[region],
            "arguments": {
                "processes": processes,
                "files": files[region],
                "out-dir": outDir,
            }
        }

        tasks.append(Task(task, "--"))


def datacard(tasks, config, channel, era, processes, mHPlus, mH, postFix = ""):
    dependencies = []
    bkgFiles, sigFiles, dataFile = {}, {}, {}
    sigName = config["signal"].format_map(dd(str, {"MHC": mHPlus, "MH": mH}))

    outDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": channel, "E": era, "R": "{}-{}".format(mHPlus, mH)}))
    systematics = config["shape-systs"].get("all", ["Nominal"]) + config["shape-systs"].get(channel, []) + config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, [])

    regions = list(set([r.split("-")[0] for r in config["regions"]]))

    for t in tasks:
        for region in regions:
            if "MergeHists_{}_{}_{}-{}-{}".format(channel, era, region, mHPlus, mH) in t["name"]:
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
                        if proc == "MisIDJ" and "_MisIDJ_" in t["name"]:
                            bkgFiles.setdefault(region, {}).setdefault(systName, []).append(t["arguments"]["out-file"])

                        elif "_{}_".format(proc) in t["name"] and systCheck:
                            bkgFiles.setdefault(region, {}).setdefault(systName, []).append(t["arguments"]["out-file"])
                            break

                    if sigName in t["name"] and systCheck:
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
            "sig-process": sigName,
            "data": config["data"].get(channel, [""])[0],
            "out-dir": outDir,
            "channel": channel,
            "era": era,
            "systematics": systematics,
            "region-names": regions,
        }
    }

    for region in task["arguments"]["region-names"]:
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
    outDir = config["dir"].format_map(dd(str, {"D": "Limit", "C": channel, "E": era, "R": "{}-{}".format(mHPlus, mH)}))
    cardDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": channel, "E": era, "R": "{}-{}".format(mHPlus, mH)}))
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

    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": "MuonIncl", "E": "2018"})))

    if "Results/Limit" in outDir:
        webDir = outDir.replace("Results", "WebPlotting/Plots").replace("LimitPDF", "")

    xSecs, limitFiles, dependencies = [], [], []

    for i, t in enumerate(tasks):
        for (mHPlus, mH) in config["masses"]:
            if i == 0:
                f = ROOT.TFile.Open("{SD}/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}_TuneCP5_PSWeights_13TeV-madgraph-pythia8/Nominal/merged/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}_TuneCP5_PSWeights_13TeV-madgraph-pythia8.root".format(SD=skimDir, MHC=mHPlus, MH=mH))
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
            "masses": ["{}-{}".format(mHPlus, mH) for (mHPlus, mH) in config["masses"]],
            "channel": channel,
            "era": era,
            "x-secs": xSecs,
            "out-dirs": [outDir, webDir] if webDir != "" else [outDir],
        }
    }

    tasks.append(Task(task, "--"))

def plotimpact(tasks, config, channel, era, mHPlus, mH, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "ImpactsPDF", "C": channel, "E": era, "R": "{}-{}".format(mHPlus, mH)}))
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
    outDir = config["dir"].format_map(dd(str, {"D": "PostfitPDF", "C": channel, "E": era, "R": "{}-{}".format(mHPlus, mH)}))
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
            "sig-process": config["signal"].format_map(dd(str, {"MHC": mHPlus, "MH": mH})),
            "out-dirs": [outDir, webDir] if webDir != "" else [outDir],
        }
    }

    tasks.append(Task(task, "--"))

def mergeDataCards(tasks, config, channels, eras, mHPlus, mH, postFix = ""):
    chanName, chanList = channels.popitem() if type(channels) == dict else (channels, [])
    eraName, eraList = eras.popitem() if type(eras) == dict else (eras, [])

    outDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": chanName, "E": eraName, "R": "{}-{}".format(mHPlus, mH)}))
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
                    if name.startswith(key):
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

                    afterExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis", "for D in $(echo {}); do gfal-mkdir -p $SRM_DESY/dbrunner/$D; while true; do gfal-copy -f -t 120 $D/{}.root $SRM_DESY/dbrunner/$D/{}.root; if [ $? -eq 0 ]; then break; fi; done; done".format(task["arguments"]["out-dir"].replace("[C]", "*").replace("[SYS]", "*"), name, name)]

                    tasks.append(Task(task, "--", beforeExe = beforeExe, afterExe = afterExe))
