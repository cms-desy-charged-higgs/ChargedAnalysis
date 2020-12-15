from task import Task

import yaml
import os
import csv

def treeread(tasks, config, channel, era, process, syst, shift):
    ##Dic with process:filenames 
    processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != ""): 
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""

    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("[C]", channel).replace("[E]", era))
        
    ## Scale systematics
    scaleSysts = config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, []) if process not in ["SingleE", "SingleMu"] and syst == "" else [""]

    fileNames = []

    ##Data driven scale factors           
    scaleFactors = ""

    if "scaleFactors" in config:
        if "Single" not in process and "HPlus" not in process:
            scaleFactors = "{}/{}".format(os.environ["CHDIR"], config["scaleFactors"].replace("[C]", channel).replace("[E]", era))

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
        for processName in processDic[process]:
            if d.startswith(processName):
                fileNames.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

    for idx, fileName in enumerate(fileNames):
        outDir = os.environ["CHDIR"] + "/{}/{}/unmerged/{}/{}".format(config["dir"].replace("[E]", era).replace("[C]", channel), process, systName, idx)

        systDirs = [outDir.replace("unmerged/", "unmerged/{}{}/".format(scaleSyst, scaleShift)) for scaleSyst in scaleSysts if scaleSyst != "" for scaleShift in ["Up", "Down"]]

        ##Configuration for treeread Task
        task = {
            "name": "TreeRead_{}_{}_{}_{}_{}".format(channel, era, process, systName, idx),
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

def mergeHists(tasks, config, channel, era, process, syst, shift):
    inputFiles, dependencies = [], []

    ##Skip Down for nominal case or and skip data if systematic
    if (syst == "" and shift == "Down") or (process in ["SingleE", "SingleMu"] and syst != ""): 
        return

    systName = "{}{}".format(syst, shift) if syst != "" else ""

    outDir = os.environ["CHDIR"] + "/{}/{}/merged/{}/".format(config["dir"].replace("[E]", era).replace("[C]", channel), process, systName)

    for t in tasks:
        if "TreeRead_{}_{}_{}_{}".format(channel, era, process, systName) in t["name"]:
            dependencies.append(t["name"])
            inputFiles.append("{}/{}".format(t["arguments"]["out-dir"], t["arguments"]["out-file"]))
    
    task = {
        "name": "MergeHists_{}_{}_{}_{}".format(channel, era, process, systName),
        "executable": "merge",
        "dir": outDir,
        "dependencies": dependencies,
        "arguments": {
            "input-files": inputFiles,
            "out-file": "{}/{}.root".format(outDir, process),
            "exclude-objects": [], 
        }
    }

    tasks.append(Task(task, "--"))
