from task import Task

import os
import yaml
import csv

class TreeRead(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "treeread"

        self["arguments"] = [
                "--parameters", *self["parameters"],
                "--cuts", *self["cuts"], 
                "--out-file", self["out-file"],  
                "--out-dir", self["dir"],  
                "--channel", self["channel"],    
                "--filename", self["filename"], 
                "--clean-jet", self["clean-jet"],
                "--scale-syst", *self["scale-syst"],    
                "--syst-dirs", *self["syst-dirs"], 
                "--scale-factors", self["scale-factors"],
                "--era", self["era"]
        ]
        
    def output(self): 
        for d in self["syst-dirs"]:
            os.makedirs(d, exist_ok = True)
    
        self["output"] = ["{}/{}".format(d, self["out-file"]) for d in [self["dir"]] + self["syst-dirs"]]

    @staticmethod
    def configure(config, channel, era, fileType="root", prefix=""):
        ##Dic with process:filenames 
        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)
        tasks = []
       
        for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []):
            for shift in ["Up", "Down"]:
                ##Skip Down for nominal case
                if syst == "" and shift == "Down":
                    continue
                systName = "{}{}".format(syst, shift) if syst != "" else ""

                skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("[C]", channel).replace("[E]", era))
        
                for process in config["processes"] + config["data"].get(channel, []):
                    if (process in ["SingleE", "SingleMu"]) and syst != "":
                        continue

                    ## Scale systematics
                    scaleSysts = config["scale-systs"].get("all", []) + config["scale-systs"].get(channel, []) if process not in ["SingleE", "SingleMu"] and syst == "" else [""]

                    jobNr = 0
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

                    for filename in fileNames:
                        outDir = os.environ["CHDIR"] + "/{}/{}/unmerged/{}/{}".format(config["dir"].replace("[E]", era).replace("[C]", channel), process, systName, jobNr).replace("//", "/")
  
                        ##Configuration for treeread Task
                        task = {
                            "name": "{}_{}_{}_{}{}".format(channel, process, era, jobNr, systName) + ("_{}".format(prefix) if prefix else ""), 
                            "display-name": "Hist: {} ({}/{})".format(process, channel, era),
                            "channel": channel, 
                            "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
                            "dir":  outDir,
                            "out-file": "{}.{}".format(process, fileType),
                            "process": process, 
                            "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
                            "filename": filename,
                            "run-mode": config["run-mode"],
                            "clean-jet": config.get("clean-jet", {}).get(channel, ""),
                            "scale-syst": scaleSysts,
                            "syst-dirs": [outDir.replace("unmerged/", "unmerged/{}{}/".format(scaleSyst, scaleShift)) for scaleSyst in scaleSysts if scaleSyst != "" for scaleShift in ["Up", "Down"]],
                            "era": era,
                            "scale-factors": scaleFactors,
                        }

                        tasks.append(TreeRead(task))

                        jobNr+=1

        return tasks
