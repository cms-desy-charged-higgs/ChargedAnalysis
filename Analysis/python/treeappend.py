from task import Task
import utils

import os
import yaml

class TreeAppend(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "treeappend"

        self["arguments"] = [
                "--old-file", self["input-file"], 
                "--old-tree", self["channel"],
                "--new-file", self["output"],
                "--branch-names", *self["branch-names"],
                "--entry-start", self["entry-start"],  
                "--entry-end", self["entry-end"],
        ]

    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["name"])
   
    @staticmethod
    def configure(config, channel):
        ##Dic with process:filenames 
        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"])
        tasks = []

        for process in config["processes"]:
            jobNr = 0

            ##List of filenames for each process
            filenames = ["{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = f) for f in processDic[process]]

            for filename in filenames:
                intervals = utils.SplitEventRange(filename, channel, config["number-events"])

                for interval in intervals:
                    ##Configuration for treeread Task
                    task = {
                              "name": "Append_{}_{}_{}".format(channel, process, jobNr), 
                              "display-name": "Append: {} ({})".format(process, channel),
                              "channel": channel, 
                              "dir":  os.environ["CHDIR"] + "/{}/{}/{}/{}".format(config["dir"], config["chan-dir"][channel], process, jobNr), 
                              "input-file": filename, 
                              "entry-start": interval[0],  
                              "entry-end": interval[1],  
                              "branch-names": config["branch-names"].get("all", []) + config["branch-names"].get(channel, []),
                              "run-mode": config["run-mode"], 
                    }

                    tasks.append(TreeAppend(task))

                    jobNr+=1

        return tasks       
