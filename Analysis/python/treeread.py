from task import Task
import utils

import os
import yaml

class TreeRead(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "treeread"

        self["arguments"] = [
                "--parameters", *self["parameters"],
                "--cuts", *self["cuts"], 
                "--out-name", self["output"],  
                "--channel", self["channel"],    
                "--filename", self["filename"], 
                "--clean-jet", self["clean-jet"], 
        ]
        
    def output(self):
        self["output"] = "{}/{}.{}".format(self["dir"], self["name"], self["file-type"])

    @staticmethod
    def configure(config, channel, fileType="root", prefix=""):
        ##Dic with process:filenames 
        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("[C]", channel).replace("[E]", config["era"]))
        tasks = []
 
        for process in config["processes"] + config["data"].get(channel, []):
            jobNr = 0
            fileNames = []

            ##List of filenames for each process
            for d in os.listdir(skimDir):
                for processName in processDic[process]:
                    if processName in d:
                        fileNames.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

            for filename in fileNames:
                ##Configuration for treeread Task
                task = {
                    "name": "{}_{}_{}_{}".format(channel, process, config["era"], jobNr) + ("_{}".format(prefix) if prefix else ""), 
                    "display-name": "Hist: {} ({}/{})".format(process, channel, config["era"]),
                    "channel": channel, 
                    "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
                    "dir":  os.environ["CHDIR"] + "/{}/{}/unmerged/{}".format(config["dir"].replace("[E]", config["era"]).replace("[C]", channel), process, jobNr), 
                    "process": process, 
                    "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
                    "filename": filename,
                    "run-mode": config["run-mode"],
                    "clean-jet": config.get("clean-jet", {}).get(channel, ""),
                    "file-type": fileType,
                }

                tasks.append(TreeRead(task))

                jobNr+=1

        return tasks
