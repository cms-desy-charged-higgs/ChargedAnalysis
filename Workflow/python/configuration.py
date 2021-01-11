from task import Task

import yaml
import os
import copy
import csv

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
            "executable": "treeread",
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

    outDir = config["dir"].format_map(dd(str, {"D": "Plots", "C": channel, "E": era})) + "/{}/{}/merged".format(process, systName)

    for t in tasks:
        if "TreeRead_{}_{}_{}_{}".format(channel, era, process, systName) in t["name"]:
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

    outDir = config["dir"].format(D = "PDF", E = era, C = channel)

    for t in tasks:
        for process in processes:
            if "MergeHists" in t["name"] and process == t["process"] and not "Up" in t["name"] and not "Down" in t["name"]:
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
            "out-dirs": [outDir],
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

def sendToDCache(config, tasks, removePrefix, postFix = ""):
    for idx, t in enumerate(copy.deepcopy(tasks)):
        taskDir = t["dir"] + "/toDCache"

        task = {
            "name": "toDache_{}_{}".format(len(tasks), postFix), 
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
        for parameter in config["parameters"]:
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
            "clean-jets": config["clean-jets"].get(channel, ""),
            "bkg-classes": list(classes.keys()),
        }
    }

    for cls, files in classes.items():
        task["arguments"]["{}-files".format(cls)] = files

    tasks.append(Task(task, "--"))
