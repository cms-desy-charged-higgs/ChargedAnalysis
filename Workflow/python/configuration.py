from task import Task

import yaml
import os
import copy
import csv
import numpy as np

import ROOT

from collections import defaultdict as dd

def sendToDCache(inFile, relativPath):
    dCachePath = "/pnfs/desy.de/cms/tier2/store/user/dbrunner/"

    command = "sendToDCache \\\n{}--input-file \\\n{}{} \\\n{}--dcache-path \\\n{}{} \\\n{}--relative-path \\\n{}{}".format(2*" ", 6*" ", inFile, 2*" ", 6*" ", dCachePath, 2*" ", 6*" ", relativPath)

    return command

def treeread(tasks, config, channel, era, process, syst, shift, mHPlus = "200", postFix = ""):
    ##Dic with process:filenames 
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != ""): 
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""

    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
        
    ## Scale systematics
    scaleSysts = config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, []) if process not in ["SingleE", "SingleMu"] and syst == "" else []

    fileNames = []

    ##Data driven scale factors           
    bkgYldFactor, bkgYldFactorSyst, bkgType = "", [], "Misc"

    if "bkg-yield-factor" in config:
        if "Single" not in process and "HPlus" not in process:
            bkgYldFactor = "{}/{}".format(os.environ["CHDIR"], config["bkg-yield-factor"].format_map(dd(str, {"C": channel, "E": era, "MHC": mHPlus, "SYS": syst, "SHIFT": shift if syst != "" else ""})))

            with open(bkgYldFactor) as f:
                reader = csv.reader(f, delimiter='\t')

                for row in reader:
                    if process in row:
                        bkgType = process
                        break

            for scaleSyst in scaleSysts:
                for scaleShift in ["Up", "Down"]:
                    bkgYldFactorSyst.append("{}/{}".format(os.environ["CHDIR"], config["bkg-yield-factor"].format_map(dd(str, {"C": channel, "E": era, "MHC": mHPlus, "SYS": scaleSyst, "SHIFT": scaleShift}))))

    ##List of filenames for each process
    for d in os.listdir(skimDir):
        for processName in processes[process]:
            if d.startswith(processName) and not "_ext" in d:
                fileNames.append("{skim}/{file}/merged/{syst}/{file}.root".format(skim=skimDir, file = d, syst = systName))

    ##List of parameters
    parameters = config["parameters"].get("all", []) + config["parameters"].get(channel, [])
    if config.get("estimate-QCD", False):
        parameters = parameters +  [p + "/" +  config.get("estimate-QCD", False) for p in parameters]

    nJobs = 0

    for fileName in fileNames:
        f = ROOT.TFile.Open(fileName)
        t = f.Get(channel)   
        nEvents = t.GetEntries()
        f.Close()

        eventRanges = [[i if i == 0 else i+1, i+config["n-events"] if i+config["n-events"] <= nEvents else nEvents] for i in range(0, nEvents, config["n-events"])]

        ##Configuration for treeread Task
        for start, end in eventRanges:
            outDir = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era})) + "/{}/unmerged/{}/{}".format(process, systName, nJobs)

            systDirs = [outDir.replace("unmerged/", "unmerged/{}{}/".format(scaleSyst, scaleShift)) for scaleSyst in scaleSysts if scaleSyst != "" for scaleShift in ["Up", "Down"]]

            ##Configuration for treeread Task
            task = {
                "name": "TreeRead_{}_{}_{}_{}_{}_{}".format(channel, era, process, systName, nJobs, postFix),
                "executable": "hist",
                "dir": outDir,
                "run-mode": config["run-mode"],
                "arguments": {
                    "filename": fileName,
                    "parameters": parameters,
                    "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
                    "out-dir": outDir,
                    "out-file": "{}.root".format(process),
                    "channel": channel,
                    "syst-dirs": systDirs,
                    "scale-systs": scaleSysts,
                    "bkg-yield-factor": bkgYldFactor,
                    "bkg-yield-factor-syst": bkgYldFactorSyst,
                    "bkg-type": bkgType,
                    "era": era,
                    "event-start": start,
                    "event-end": end
                }
            }

            tasks.append(Task(task, "--"))

            nJobs += 1

def mergeHists(tasks, config, channel, era, process, syst, shift, postFix = ""):
    inputFiles, dependencies = [], []

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != ""): 
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""

    ##Output directory
    outDir = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era})) + "/{}/merged/{}".format(process, systName)

    for t in tasks:
        if "TreeRead_{}_{}_{}".format(channel, era, process) in t["name"] and postFix in t["name"]:
            if systName != "" and systName in t["name"]:
                dependencies.append(t["name"])
                inputFiles.append("{}/{}".format(t["arguments"]["out-dir"], t["arguments"]["out-file"]))
                
            elif syst in t["arguments"]["scale-systs"]:
                dependencies.append(t["name"])
                inputFiles.append("{}/{}".format(t["arguments"]["out-dir"].replace("unmerged/", "unmerged/" + systName), t["arguments"]["out-file"]))

            elif not "Up" in t["name"] and not "Down" in t["name"] and syst == "":
                dependencies.append(t["name"])
                inputFiles.append("{}/{}".format(t["arguments"]["out-dir"].replace("unmerged/", "unmerged/" + systName), t["arguments"]["out-file"]))

    task = {
        "name": "MergeHists_{}_{}_{}_{}_{}".format(channel, era, process, systName, postFix),
        "executable": "merge",
        "dir": outDir,
        "dependencies": dependencies,
        "process": process,
        "arguments": {
            "input-files": inputFiles,
            "out-file": "{}/{}.root".format(outDir, process),
            "exclude-objects": [], 
        }
    }

    tasks.append(Task(task, "--"))

def plotting(tasks, config, channel, era, processes, postFix = ""):
    dependencies = []
    bkgProcesses, sigProcesses = [], []
    bkgFiles, sigFiles = {}, {}
    data, dataFile = "", ""

    processes.sort()

    outDir = config["dir"].format_map(dd(str, {"D": "PDF", "C": channel, "E": era}))
    webDir = ""

    if "Results/Plot" in outDir:
        webDir = outDir.replace("Results/Plot", "WebPlotting/Plots").replace("PDF", "")

    systematics = config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []) + config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, [])

    for t in tasks:
        for process in processes:
            if "MergeHists_{}_{}_{}_".format(channel, era, process) in t["name"] and postFix in t["name"]:
                dependencies.append(t["name"])

            elif process == "QCD" and "QCDEstimate_{}_{}".format(channel, era) in t["name"] and postFix in t["name"]:
                dependencies.append(t["name"])

            else:
                continue

            if "Single" in process:
                data = process
                dataFile = t["arguments"]["out-file"]
                    
            for syst in systematics:
                for shift in ["Up", "Down"]:
                    if syst == "" and shift == "Down":
                        continue

                    systName = "{}{}".format(syst, shift) if syst != "" else ""
                    systCheck = systName in t["name"] if syst != "" else not "Up" in t["name"] and not "Down" in t["name"]

                    if "HPlus" in process and systName in t["name"] and systCheck:
                        if syst == "":
                            sigProcesses.append(process)
                        sigFiles.setdefault(systName, []).append(t["arguments"]["out-file"])

                    elif not "Single" in process and systName in t["name"] and systCheck:
                        if syst == "":
                            bkgProcesses.append(process)
                        bkgFiles.setdefault(systName, []).append(t["arguments"]["out-file"])

    task = {
        "name": "Plot_{}_{}_{}".format(channel, era, postFix), 
        "executable": "plot",
        "dir": outDir,
        "dependencies": dependencies,
        "arguments": {
            "channel": channel,
            "era": era,
            "bkg-processes": bkgProcesses,
            "bkg-files": bkgFiles.get("", []),
            "sig-processes": sigProcesses,
            "sig-files": sigFiles.get("", []),
            "data": data,
            "data-file": dataFile,
            "out-dirs": [outDir, webDir] if webDir != "" else [outDir],
            "systematics": ['""'] + systematics[1:]
        }
    }

    for syst in systematics[1:]:
        for shift in ["Up", "Down"]:
            systName = "{}{}".format(syst, shift)

            if task["arguments"]["bkg-files"]:
                task["arguments"]["bkg-files-{}{}".format(syst, shift)] = bkgFiles[systName]

            if task["arguments"]["sig-files"]:
                task["arguments"]["sig-files-{}{}".format(syst, shift)] = sigFiles[systName]

    tasks.append(Task(task, "--"))

def treeappend(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    outDir = config["dir"].format_map(dd(str, {"C": channel, "E": era}))

    for fileName in os.listdir(skimDir):
        if "Run2" in fileName and syst != "":
            continue

        ##Ignore extensions
        if "_ext" in fileName:
            continue

        ##Configuration for treeread Task
        task = {
            "name": "Append_{}_{}_{}_{}_{}".format(channel, era, fileName, systName, postFix),
            "dir":  "{}/{}/{}".format(outDir, fileName, systName),
            "executable": "treeappend",
            "run-mode": config["run-mode"],
            "arguments": {
                "out-file": "{}/{}/merged/{}/{}.root".format(skimDir, fileName, systName, fileName),
                "tree-name": channel,
                "functions": config["functions"].get("all", []) + config["functions"].get(channel, []),
                "era": era,
            }
        }

        tasks.append(Task(task, "--"))

def stitching(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))

    filesToSwitch = {}

    for fileName in os.listdir(skimDir):
        ##Ignore extensions
        if "ext" in fileName:
            continue

        for name, filePrefixes in config["files"].items():
            for f in filePrefixes:
                if fileName.startswith(f):
                    filesToSwitch.setdefault(name, []).append("{}/{}/merged/{}/{}.root".format(skimDir, fileName, systName, fileName))

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

        tasks.append(Task(task, "--"))

def QCDEstimation(tasks, config, channel, era, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""
    outDir = config["dir"].format_map(dd(str, {"D": "QCDEstimation", "C": channel, "E": era})) + "/" + systName
    data = config["data"][channel][0]

    depTasks = []
    depFiles = {}
    regions = ["A", "B", "C", "E", "F", "G", "H"]

    for t in tasks:
        for region in regions:
            if "MergeHists_{}_{}_".format(channel, era) in t["name"] and "QCD-{}".format(region) in t["name"]:
                if systName in t["name"] and systName != "":
                    depTasks.append(t)
                    depFiles.setdefault(region, {}).setdefault(t["process"], t["arguments"]["out-file"])

                elif data in t["name"]:
                    depTasks.append(t)
                    depFiles.setdefault(region, {}).setdefault("data", t["arguments"]["out-file"])

                elif syst == "" and not "Up" in t["name"] and not "Down" in t["name"]:
                    depTasks.append(t)
                    depFiles.setdefault(region, {}).setdefault(t["process"], t["arguments"]["out-file"])

                break

    task = {
        "name": "QCDEstimate_{}_{}_{}_{}".format(channel, era, systName, postFix),
        "dir": outDir,
        "executable": "estimateQCD",
        "dependencies": [t["name"] for t in depTasks],
        "arguments": {
            "out-file": "{}/QCD.root".format(outDir),
            "processes": [proc for proc in config["processes"] if proc != "QCD"] + ["data"],
        }
    }
          
    for process in task["arguments"]["processes"]:
        for region in regions:
            task["arguments"]["{}-{}-file".format(region, process)] = depFiles[region][process]
    
    tasks.append(Task(task, "--"))

def bkgEstimation(tasks, config, channel, era, mass, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""
    outDir = config["dir"].format_map(dd(str, {"D": "ScaleFactor", "C": channel, "E": era, "MHC": mass})) + "/" + systName
    data = config["data"][channel][0]

    depTasks = []

    for t in tasks:
        if "MergeHists_{}_{}_".format(channel, era) in t["name"]:
            if mass in t["name"]:
                if systName in t["name"] and systName != "":
                    depTasks.append(t)

                elif data in t["name"]:
                    depTasks.append(t)

                elif syst == "" and not "Up" in t["name"] and not "Down" in t["name"]:
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

def treeslim(tasks, config, inChannel, outChannel, era, syst, shift, postFix = ""):
    ##Construct systname and skim dir
    systName = "" if syst == "" else "{}{}".format(syst, shift)
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"E": era})))

    ##If nominal skip Down loop
    if(syst == "" and shift == "Down"):
        return

    for d in os.listdir(skimDir):
        ##Skip if data and not nominal
        if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "":
            continue

        ##Ignore extensions
        if "_ext" in d:
            continue

        inFile = "{}/{}/merged/{}/{}.root".format(skimDir, d, systName, d)

        if not os.path.exists(inFile):
            raise FileNotFoundError("Not existing: {}".format(inFile))

        f = ROOT.TFile.Open(inFile)
        t = f.Get(inChannel)   
        nEvents = t.GetEntries()
        f.Close()

        eventRanges = [[i if i == 0 else i+1, i+config["n-events"] if i+config["n-events"] <= nEvents else nEvents] for i in range(0, nEvents, config["n-events"])]

        ##Configuration for treeread Task   
        for idx, (start, end) in enumerate(eventRanges):
            outDir = "{}/{}/unmerged/{}/{}".format(config["out-dir"].format_map(dd(str, {"D": "Histograms", "C": outChannel, "E": era})), d, systName, idx)
            dir = "{}/{}/unmerged/{}/{}".format(config["dir"].format_map(dd(str, {"C": outChannel, "E": era})), d, systName, idx)

            ##Configuration for treeread Task
            task = {
                "name": "TreeSlim_{}_{}_{}_{}_{}_{}_{}".format(inChannel, outChannel, era, d, systName, idx, postFix),
                "executable": "treeslim",
                "run-mode": config["run-mode"],
                "dir": dir,
                "arguments": {
                    "input-file": inFile,
                    "input-channel": inChannel,
                    "out-channel": outChannel, 
                    "cuts": config["cuts"].get(outChannel, []),
                    "out-file": "{}/{}/{}.root".format(os.environ["CHDIR"], outDir, d), 
                    "event-start": start,
                    "event-end": end,
                }
            }


            beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
            afterExe = [sendToDCache(task["arguments"]["out-file"], outDir)]

            tasks.append(Task(task, "--", beforeExe, afterExe))

def mergeFiles(tasks, config, inChannel, outChannel, era, syst, shift, postFix = ""):
    ##Construct systname and skim dir
    systName = "" if syst == "" else "{}{}".format(syst, shift)
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": outChannel, "E": era})))

    ##If nominal skip Down loop
    if(syst == "" and shift == "Down"):
        return

    for d in os.listdir(skimDir):
        inputFiles, dependencies = [], []

        if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "":
            continue

        ##Ignore extensions
        if "_ext" in d:
            continue

        dir = "{}/{}/merged/{}".format(config["dir"].format_map(dd(str, {"D": "Histograms", "C": outChannel, "E": era})), d, systName)
        outDir = "{}/{}/{}/merged/{}".format(config["out-dir"].format_map(dd(str, {"D": "Histograms", "C": outChannel, "E": era})), d, systName)

        for t in tasks:
            if "TreeSlim_{}_{}_{}_{}_{}".format(inChannel, outChannel, era, d, systName) in t["name"] and postFix in t["name"]:
                dependencies.append(t["name"])
                inputFiles.append(t["arguments"]["out-file"])
        
        task = {
            "name": "MergeHists_{}_{}_{}_{}_{}".format(outChannel, era, d, systName, postFix),
            "executable": "merge",
            "dir": dir,
            "dependencies": dependencies,
            "arguments": {
                "input-files": inputFiles,
                "out-file": "{}/{}/{}.root".format(os.environ["CHDIR"], outDir, d),
                "exclude-objects": config["exclude-merging"],
                "optimize": "",
            }
        }

        beforeExe = ["source $CHDIR/ChargedAnalysis/setenv.sh Analysis"]
        afterExe = [sendToDCache(task["arguments"]["out-file"], outDir)]

        tasks.append(Task(task, "--", beforeExe, afterExe))

def mergeSkim(tasks, config, channel, era, syst, shift, postFix = ""):
    skimDir = config["skim-dir"].format_map(dd(str, {"C": channel, "E": era}))
    absSkimDir = "{}/{}".format(os.environ["CHDIR"], skimDir)
    groupDir = "/nfs/dust/cms/group/susy-desy/david/MergeSkim/"

    toMerge = {}
    systName = "{}{}".format(syst, shift) if syst != "" else ""

    for d in os.listdir(absSkimDir):
        ##Skip if data and not nominal
        if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "":
            continue

        ##Skip Down for nominal case
        if syst == "" and shift == "Down":
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

def DNN(tasks, config, channel, era, evType, postFix = ""):
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))

    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    outDir = config["dir"].format_map(dd(str, {"C": channel, "E": era, "T": evType}))

    classes, signals = {}, []

    for d in sorted(os.listdir(skimDir)):
        for process in config["classes"] + config.get("Misc", []):
            for processName in processes[process]:
                if d.startswith(processName):
                    cls = process if process in config["classes"] else "Misc"
                        
                    classes.setdefault(cls, []).append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

        for process in [config["signal"].format_map(dd(str, {"MHC": mass})) for mass in config["masses"]]:
            for processName in processes[process]:
                if d.startswith(processName):
                    signals.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

    os.makedirs(outDir, exist_ok=True)

    with open(outDir + "/parameter.txt", "w") as param:
        for parameter in config["parameters"].get("all", []) + config["parameters"].get(channel, []):
            param.write(parameter + "\n")
                
    with open(outDir + "/classes.txt", "w") as param:
        for cls in classes.keys():
            param.write(cls + "\n")
                
    with open(outDir + "/masses.txt", "w") as param:
        for mass in config["masses"]:
            param.write(str(mass) + "\n")

    task = {
        "name": "DNN_{}_{}_{}_{}".format(channel, era, evType, postFix),
        "executable": "dnn",
        "dir": outDir,
        "run-mode": config["run-mode"],
        "arguments": {
            "out-path": outDir,
            "channel": channel,   
            "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
            "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
            "sig-files": signals,
            "masses": config["masses"],
            "opt-param": config.get("hyper-opt", "").format_map(dd(str, {"C": channel, "E": era, "T": evType})),
            "era": era,
            "bkg-classes": list(classes.keys()),
        }
    }

    if evType == "Even":
        task["arguments"]["is-even"] = ""

    for cls, files in classes.items():
        task["arguments"]["{}-files".format(cls)] = files

    tasks.append(Task(task, "--"))

def eventCount(tasks, config, channel, era, processes, syst, shift, postFix = ""):
    ##Skip Down for nominal case
    if syst == "" and shift == "Down":
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""

    outDir = config["dir"].format_map(dd(str, {"D": "EventCount", "C": channel, "E": era})) + "/" + systName
    files, dependencies = [], []

    for process in processes:
       for t in tasks:
            if "MergeHists_{}_{}_{}_".format(channel, era, process) in t["name"] and postFix in t["name"]:
                if systName in t["name"] and systName != "":
                    dependencies.append(t["name"])
                    files.append(t["arguments"]["out-file"])

                elif syst == "" and not "Up" in t["name"] and not "Down" in t["name"]:
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
                    if syst == "" and shift == "Down":
                        continue

                    systName = "{}{}".format(syst, shift) if syst != "" else ""
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
                if syst == "" and shift == "Down":
                    continue

                systName = "{}{}".format(syst, shift) if syst != "" else ""

                task["arguments"]["bkg-files-{}".format(region) + ("-{}".format(systName) if systName != "" else "")] = bkgFiles[region][systName]
                task["arguments"]["sig-files-{}".format(region) + ("-{}".format(systName) if systName != "" else "")] = sigFiles[region][systName]

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
