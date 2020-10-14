from task import Task

import os
import yaml

class Merge(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "merge"

        self["arguments"] = [
                "--input-files", *self["input-files"],
                "--out-file", self["output"], 
                "--exclude-objects", *self["exclude-objects"],
                "--delete-input" if "delete-input" in self else "",
        ]
        
    def output(self):
        self["output"] = "{}/{}".format(self["dir"], self["out-name"])

    @staticmethod
    def configure(config, dependentTasks, taskName, prefix = "", **kwargs):
        tasks = []

        if taskName == "plot":
            unmergedDirs = list(set(["/".join(d.split("/")[:-1]) for task in dependentTasks for d in [task["dir"]] + task["syst-dirs"]]))
            toMerge = [[task for task in dependentTasks if True in [d == "/".join(out.split("/")[:-1]) for out in [task["dir"]] + task["syst-dirs"]]] for d in unmergedDirs]

            for index, d in enumerate(unmergedDirs):
                channel = toMerge[index][0]["channel"]
                process = toMerge[index][0]["process"]
                systName = d.split("/")[-1]

                task = {
                    "name": "Merge_{}_{}_{}{}".format(process, channel, kwargs["era"], systName) + ("_{}".format(prefix) if prefix else ""),  
                    "dir": d.replace("unmerged", "merged"),
                    "channel": channel,
                    "process": process,
                    "out-name": "{}.root".format(process), 
                    "display-name": "Merge: {} ({}/{})".format(process, channel, kwargs["era"]),
                    "dependencies": [t["name"] for t in toMerge[index]],
                    "exclude-objects": [], 
                    "input-files": ["{}/{}/{}.root".format(d, i, process) for i in range(len(toMerge[index]))]
                }

                tasks.append(Merge(task))
            return tasks

        if taskName == "slim":
            unmergedDirs = list(set(["/".join(task["dir"].split("/")[:-1]) for task in dependentTasks]))
            toMerge = [[task for task in dependentTasks if d in task["dir"] and task["systematic"] in d] for d in unmergedDirs]

            for index, mergeTasks in enumerate(toMerge):
                task = {
                    "name": "Merge_{}".format(index), 
                    "display-name": "Merge: {}".format(mergeTasks[0]["out-channel"]),
                    "out-name": mergeTasks[0]["out-name"], 
                    "dir": unmergedDirs[index].replace("unmerged", "merged"),
                    "dependencies": [t["name"] for t in mergeTasks],
                    "input-files": ["{}/{}".format(t["dir"], t["out-name"]) for t in mergeTasks],
                    "exclude-objects": ["Lumi", "xSec", "pileUp", "pileUpUp", "pileUpDown", "nGen", "nGenWeighted", "puMC", "nLooseDeepbTag", "nLooseDeepbTag", "nMediumDeepbTag", "nTightDeepbTag", "nLooseCSVbTag", "nMediumCSVbTag", "nTightCSVbTag", "nTrueB"],
                    "delete-input": True,
                    "run-mode": config["run-mode"],
                }

                tasks.append(Merge(task))            
            
        return tasks
