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
                "--event-yield", *self["interval"]
        ]
        
    def output(self):
        self["output"] = "{}/{}.{}".format(self["dir"], self["name"], self["file-type"])

    @staticmethod
    def configure(config, channel, fileType="root", prefix=""):
        ##Dic with process:filenames 
        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"])
        tasks = []
 
        for process in config["processes"] + config["data"].get(channel, []):
            jobNr = 0

            ##List of filenames for each process
            filenames = ["{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = f) for f in processDic[process]]

            for filename in filenames:
                intervals = utils.SplitEventRange(filename, channel, config["number-events"])

                for interval in intervals:
                    ##Configuration for treeread Task
                    task = {
                            "name": "{}_{}_{}".format(channel, process, jobNr) + ("_{}".format(prefix) if prefix else ""), 
                            "display-name": "Hist: {} ({})".format(process, channel),
                            "channel": channel, 
                            "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
                            "dir":  os.environ["CHDIR"] + "/{}/{}/{}/unmerged/{}".format(config["dir"], channel, process, jobNr), 
                            "process": process, 
                            "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
                            "filename": filename,
                            "interval": interval,
                            "run-mode": config["run-mode"],
                            "clean-jet": config.get("clean-jet", {}).get(channel, ""),
                            "file-type": fileType,
                    }

                    tasks.append(TreeRead(task))

                    jobNr+=1

        return tasks
