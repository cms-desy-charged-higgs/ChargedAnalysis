from task import Task

import yaml
import os
import copy
import csv

import ROOT

from collections import defaultdict as dd

def treeread(tasks, config, channel, era, process, syst, shift, postFix = ""):
    ##Dic with process:filenames 
    processes = yaml.safe_load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"))

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != ""): 
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""

    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
        
    ## Scale systematics
    scaleSysts = config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, []) if process not in ["SingleE", "SingleMu"] and syst == "" else [""]

    fileNames = []

    ##Data driven scale factors           
    scaleFactors = ""

    if "scaleFactors" in config:
        if "Single" not in process and "HPlus" not in process:
            scaleFactors = "{}/{}".format(os.environ["CHDIR"], config["scaleFactors"].format_map(dd(str, {"C": channel, "E": era})))

            with open(scaleFactors) as f:
                reader = csv.reader(f, delimiter='\t')

                toAppend = "+Misc"

                for row in reader:
                    if process in row:
                        toAppend = "+" + process
                        break 

                scaleFactors += toAppend

    ##List of filenames for each process
    for d in os.listdir(skimDir):
        for processName in processes[process]:
            if d.startswith(processName):
                fileNames.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

    for idx, fileName in enumerate(fileNames):
        outDir = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era})) + "/{}/{}/unmerged/{}".format(process, systName, idx)

        systDirs = [outDir.replace("unmerged/", "unmerged/{}{}/".format(scaleSyst, scaleShift)) for scaleSyst in scaleSysts if scaleSyst != "" for scaleShift in ["Up", "Down"]]

        ##Configuration for treeread Task
        task = {
            "name": "TreeRead_{}_{}_{}_{}_{}_{}".format(channel, era, process, systName, idx, postFix),
            "executable": "hist",
            "dir": outDir,
            "run-mode": config["run-mode"],
            "arguments": {
                "filename": fileName,
                "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
                "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
                "out-dir": outDir,
                "out-file": "{}.root".format(process),
                "channel": channel,
                "clean-jet": config.get("clean-jet", {}).get(channel, ""),
                "syst-dirs": systDirs,
                "scale-systs": scaleSysts,
                "scale-factors": scaleFactors,
                "era": era
            }
        }

        tasks.append(Task(task, "--"))

def mergeHists(tasks, config, channel, era, process, syst, shift, postFix = ""):
    inputFiles, dependencies = [], []

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != ""): 
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""

    outDir = config["dir"].format_map(dd(str, {"D": "Histograms", "C": channel, "E": era})) + "/{}/{}/merged".format(process, systName)

    for t in tasks:
        if "TreeRead_{}_{}_{}_{}".format(channel, era, process, systName) in t["name"] and postFix in t["name"]:
            dependencies.append(t["name"])
            inputFiles.append("{}/{}".format(t["arguments"]["out-dir"], t["arguments"]["out-file"]))
    
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
    bkgFiles, sigFiles = [], []
    data, dataFile = "", ""

    processes.sort()

    outDir = config["dir"].format_map(dd(str, {"D": "PDF", "C": channel, "E": era}))
    webDir = ""

    if "Results/Plot" in outDir:
        webDir = outDir.replace("Results/Plot", "CernWebpage/Plots").replace("PDF", "")

    for t in tasks:
        for process in processes:
            if "MergeHists_{}_{}_{}_".format(channel, era, process) in t["name"] and postFix in t["name"]:
                dependencies.append(t["name"])

                if "HPlus" in process:
                    sigProcesses.append(process)
                    sigFiles.append(t["arguments"]["out-file"])

                elif "Single" in process:
                    data = process
                    dataFile = t["arguments"]["out-file"]

                else:
                    bkgProcesses.append(process)
                    bkgFiles.append(t["arguments"]["out-file"])
                    

    task = {
        "name": "Plot_{}_{}_{}".format(channel, era, postFix), 
        "executable": "plot",
        "dir": outDir,
        "dependencies": dependencies,
        "arguments": {
            "channel": channel,
            "era": era,
            "bkg-processes": bkgProcesses,
            "bkg-files": bkgFiles,
            "sig-processes": sigProcesses,
            "sig-files": sigFiles,
            "data": data,
            "data-file": dataFile,
            "out-dirs": [outDir, webDir] if webDir != "" else [outDir],
        }
    }

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

def sendToDCache(tasks, config, removePrefix, postFix = ""):
    for idx, t in enumerate(copy.deepcopy(tasks)):
        taskDir = t["dir"] + "/toDCache"

        task = {
            "name": "toDache_{}_{}".format(idx, postFix), 
            "dir": taskDir,
            "executable": "sendToDCache",
            "dependencies": [t["name"]],
            "arguments": {
                "input-file": t["arguments"]["out-file"],
                "out-file": t["arguments"]["out-file"].split("/")[-1], 
                "dcache-path": "/pnfs/desy.de/cms/tier2/store/user/dbrunner/",
                "relative-path": "/".join(t["arguments"]["out-file"].split("/")[:-1]).replace(removePrefix, ""), 
            }
        }

        tasks.append(Task(task, "--"))

def bkgEstimation(tasks, config, channel, era, mass, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "ScaleFactor", "C": channel, "E": era, "MHC": mass}))
    data = config["data"][channel][0]

    depTasks = [t for t in tasks if "MergeHists_{}_{}".format(channel, era) in t["name"] and mass in t["name"]]

    task = {
        "name": "Estimate_{}_{}_{}".format(channel, era, mass, postFix),
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
    tmpDir = "/nfs/dust/cms/group/susy-desy/david/Slim/{}/{}".format(outChannel, era)
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"E": era})))

    ##If nominal skip Down loop
    if(syst == "" and shift == "Down"):
        return

    for d in os.listdir(skimDir):
        ##Skip if data and not nominal
        if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "":
            continue

        inFile = "{}/{}/merged/{}/{}.root".format(skimDir, d, systName, d)

        if not os.path.exists(inFile):
            raise FileNotFoundError("File not found: " +  inFile)

        f = ROOT.TFile.Open(inFile)
        t = f.Get(inChannel)   
        nEvents = t .GetEntries()

        eventRanges = [[i if i == 0 else i+1, i+config["n-events"] if i+config["n-events"] <= nEvents else nEvents] for i in range(0, nEvents, config["n-events"])]

        ##Configuration for treeread Task   
        for idx, (start, end) in enumerate(eventRanges):
            outDir = "{}/{}/unmerged/{}/{}".format(tmpDir, d, systName, idx)
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
                    "out-file": "{}/{}.root".format(outDir, d), 
                    "event-start": start,
                    "event-end": end,
                }
            }

            tasks.append(Task(task, "--"))

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

        dir = "{}/{}/merged/{}".format(config["dir"].format_map(dd(str, {"D": "Histograms", "C": outChannel, "E": era})), d, systName)
        outDir = "{}/{}/{}/merged/{}".format(os.environ["CHDIR"], config["out-dir"].format_map(dd(str, {"D": "Histograms", "C": outChannel, "E": era})), d, systName)

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
                "out-file": "{}/{}.root".format(outDir, d),
                "exclude-objects": config["exclude-merging"],
                "delete-input": "",
            }
        }

        tasks.append(Task(task, "--"))

def mergeSkim(tasks, config, channel, era, syst, shift, postFix = ""):
    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].format_map(dd(str, {"C": channel, "E": era})))
    tmpDir = "/nfs/dust/cms/group/susy-desy/david/MergeSkim/{}".format(era)

    ##Loop over all directories in the skim dir
    for d in os.listdir(skimDir):
        mergeFiles = {}

        ##Skip if data and not nominal
        if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "":
            continue

        ##Skip Down for nominal case
        if syst == "" and shift == "Down":
            continue

        systName = "{}{}".format(syst, shift) if syst != "" else ""

        ##Open file with xrootd paths to output files
        with open("{}/{}/outputFiles.txt".format(skimDir, d)) as fileList:
            files, tmpTask = [], []
            nFilesMerge = 30

            nFiles = len(list(fileList))
            fileList.seek(0)

            for idx, f in enumerate(fileList):
                fileName = ""

                if syst != "":
                    if systName in f:
                        fileName = f.replace("\n", "")

                else:
                    if "Up" not in f and "Down" not in f:
                        fileName = f.replace("\n", "")

                if(fileName != ""):
                    files.append(fileName)

                if (len(files) > nFilesMerge or idx == nFiles - 1) and len(files) != 0:
                    task = {
                        "name": "MergeSkim_{}_{}_{}_{}_{}_{}".format(channel, era, d, systName, len(tmpTask), postFix),
                        "dir": "{}/{}/tmp/{}/{}".format(config["dir"].format_map(dd(str, {"C": channel, "E": era})), d, systName, len(tmpTask)),
                        "executable": "merge",
                        "run-mode": config["run-mode"],
                        "arguments": {
                            "input-files": copy.deepcopy(files),                  
                            "out-file": "{}/{}/tmp/{}/{}/{}.root".format(tmpDir, d, systName, len(tmpTask), d),
                            "exclude-objects": ["Lumi", "xSec", "pileUp", "pileUpUp", "pileUpDown"],
                        }      
                    }
  
                    tmpTask.append(Task(task, "--"))
                    files.clear()

            task = {
                "name": "MergeSkim_{}_{}_{}_{}_{}".format(channel, era, d, systName, postFix),
                "dir": "{}/{}/merged/{}".format(config["dir"].format_map(dd(str, {"C": channel, "E": era})), d, systName),
                "executable": "merge",
                "run-mode": config["run-mode"],
                "dependencies": [t["name"] for t in tmpTask],
                "arguments": {
                    "input-files": [t["arguments"]["out-file"] for t in tmpTask],
                    "out-file": "{}/{}/merged/{}/{}.root".format(skimDir, d, systName, d),
                    "exclude-objects": ["Lumi", "xSec", "pileUp", "pileUpUp", "pileUpDown"],
                    "delete-input": "",
                }
            }

            tasks.extend(tmpTask)
            tasks.append(Task(task, "--"))

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

def eventCount(tasks, config, channel, era, processes, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "EventCount", "C": channel, "E": era}))
    files, dependencies = [], []

    for process in processes:
       for t in tasks:
            if "MergeHists_{}_{}_{}_".format(channel, era, process) in t["name"] and postFix in t["name"]:
                dependencies.append(t["name"])
                files.append(t["arguments"]["out-file"])
                    
    task = {
        "name": "EventCount_{}_{}_{}".format(channel, era, postFix), 
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
    bkgFiles, sigFiles = [], []
    dataFile = ""

    outDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": channel, "E": era}))

    for t in tasks:
        if "MergeHists_{}_{}".format(channel, era) in t["name"] and "{}_{}".format(mHPlus, mH) in t["name"]:
            dependencies.append(t["name"])

            for proc in config["backgrounds"]:
                if proc in t["name"]:
                    bkgFiles.append(t["arguments"]["out-file"])
                    break

            if config["signal"] in t["name"]:
                sigFiles.append(t["arguments"]["out-file"])

            if config["data"].get("channel", "") != "" and config["data"].get("channel", "") in t["name"]:
                dataFile = t["arguments"]["out-file"]
                   
    task = {
        "name": "DataCard_{}_{}_{}_{}_{}".format(channel, era, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "writecard",
        "dependencies": dependencies,
        "arguments": {
            "discriminant": config["discriminant"],
            "bkg-processes": config["backgrounds"],
            "sig-process": config["signal"],
            "data": config["data"].get("channel", ""),
            "bkg-files": bkgFiles,
            "sig-files": sigFiles,
            "data-file": dataFile,
            "out-dir": outDir,
            "channel": channel,
            "systematics": ['""'] + config["shape-systs"].get("all", [])[1:] + config["shape-systs"].get(channel, []) + config["scale-systs"].get("all", [])[1:] + config["scale-systs"].get(channel, []),
        }
    }

    tasks.append(Task(task, "--"))

def limits(tasks, config, channel, era, mHPlus, mH, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "Limit", "C": channel, "E": era}))
    cardDir = config["dir"].format_map(dd(str, {"D": "DataCard", "C": channel, "E": era}))

    task = {
        "name": "Limit_{}_{}_{}_{}_{}".format(channel, era, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "combine",
        "dependencies": [t["name"] for t in tasks if t["dir"] == cardDir],
        "arguments": {
            "datacard": "{}/datacard.txt".format(cardDir),   
            "saveWorkspace": "",
        }
    }

    tasks.append(Task(task, "--", ["source $CHDIR/ChargedAnalysis/setenv.sh CMSSW", "cd {}".format(outDir)], ["mv higgs*.root limit.root"]))

def plotlimit(tasks, config, channel, era, postFix = ""):
    outDir = config["dir"].format_map(dd(str, {"D": "LimitPDF", "C": channel, "E": era}))
    webDir = ""

    if "Results/Limit" in outDir:
        webDir = outDir.replace("Results", "CernWebpage/Plots").replace("LimitPDF", "")

    xSecs, limitFiles, dependencies = [], [], []

    for mHPlus in config["charged-masses"]:
        for mH in config["neutral-masses"]:
            f = ROOT.TFile.Open("$CHDIR/Skim/Inclusive/2016/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}/merged/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}.root".format(MHC=mHPlus, MH=mH))
            xSecs.append(f.Get("xSec").GetBinContent(1))

    for t in tasks:
        for mHPlus in config["charged-masses"]:
            for mH in config["neutral-masses"]:
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
                    args += "{}={} ".format(chan + "_" + era, "{}/datacard.txt".format(t["dir"]))

    task = {
        "name": "MergeCards_{}_{}_{}_{}_{}".format(chanName, eraName, mHPlus, mH, postFix), 
        "dir": outDir,
        "executable": "combineCards.py",
        "dependencies": dependencies,
        "arguments": {
            args + " > {}/datacard.txt".format(outDir) : "",
        }
    }

    tasks.append(Task(task, "", ["source $CHDIR/ChargedAnalysis/setenv.sh CMSSW"]))


