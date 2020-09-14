from task import Task

import os
import yaml

class Merge(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "merge"

        self["arguments"] = [
                "--input-files", *self["dependent-files"],
                "--out-file", self["output"], 
                "--exclude-objects", *self["exclude-objects"],
                "--delete-input" if "delete-input" in self else "",
        ]
        
    def output(self):
        self["output"] = "{}/{}".format(self["dir"], self["out-name"])

    @staticmethod
    def configure(config, dependentTasks, taskName, prefix = ""):
        tasks = []

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
                    "exclude-objects": ["Lumi", "xSec", "pileUp", "pileUpUp", "pileUpDown", "nGen", "nGenWeighted", "puMC", "nLooseDeepbTag", "nLooseDeepbTag", "nMediumDeepbTag", "nTightDeepbTag", "nLooseCSVbTag", "nMediumCSVbTag", "nTightCSVbTag", "nTrueB"],
                    "delete-input": True,
                }

                tasks.append(Merge(task))            
            
        return tasks
