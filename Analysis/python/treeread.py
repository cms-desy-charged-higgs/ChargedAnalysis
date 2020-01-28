from task import Task
import utils

import os
import yaml
import numpy as np

class TreeRead(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "TreeRead"

        self["arguments"] = [
                "--process", self["process"], 
                "--x-parameters", *self["x-parameters"],
                "--y-parameters", *self["y-parameters"],
                "--cuts", *self["cuts"], 
                "--out-name", self["output"],  
                "--channel", self["channel"],    
                "--save-mode", self["save-mode"],
                "--filename", self["filename"], 
                "--event-yield", *self["interval"]
        ]
        
    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["name"])

    @staticmethod
    def configure(config, channel, prefix=""):
        nEvents = config["number-events"]

        ##Dic with process:filenames 
        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        skimDir = os.environ["CHDIR"] + "/Skim"
        tasks = []
 
        for process in config["processes"] + config["data"].get(channel, []):
            ##List of filenames for each process
            filenames = ["{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = f) for f in processDic[process]]

            for filename in filenames:
                intervals = utils.SplitEventRange(filename, channel, nEvents)

                for interval in intervals:
                    ##Configuration for treeread Task
                    task = {
                            "name": "{}_{}_{}_{}".format(config["save-mode"], channel, process, len(tasks)) + ("_{}".format(prefix) if prefix else ""), 
                            "display-name": "Hist: {} ({})".format(process, channel),
                            "channel": channel, 
                            "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
                            "dir":  os.environ["CHDIR"] + "/Tmp/{}/{}/{}".format(config["save-mode"], config["dir"], config["chan-dir"][channel]), 
                            "process": process, 
                            "x-parameters": config["x-parameters"].get("all", []) + config["x-parameters"].get(channel, []),
                            "y-parameters": config["y-parameters"].get("all", []) + config["y-parameters"].get(channel, []),
                            "filename": filename,
                            "interval": interval,
                            "run-mode": config["run-mode"], 
                            "save-mode": config["save-mode"], 
                    }

                    tasks.append(TreeRead(task))

        return tasks
