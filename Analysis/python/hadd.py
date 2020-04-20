from task import Task

import os

class HaddPlot(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "hadd"
        self["arguments"] =  [
                    "-f",
                    self["output"],   
                    *self["dependent-files"]
        ]

    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["process"])

    @staticmethod
    def configure(config, treeTasks, channel, prefix=""):
        tasks = []

        for process in config["processes"] + config["data"].get(channel, []):
            outDir = os.environ["CHDIR"] + "/{}/{}/{}".format(config["dir"], channel, process)
                
            task = {
                    "name": "Hadd_{}_{}".format(process, channel) + ("_{}".format(prefix) if prefix else ""),  
                    "dir": outDir,
                    "channel": channel,
                    "process": process,
                    "display-name": "Hadd: {} ({})".format(process, channel),
                    "dependencies": [t["name"] for t in treeTasks if process == t["process"] and channel == t["channel"]]
            }

            tasks.append(HaddPlot(task))
        return tasks


class HaddAppend(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "hadd"
        self["arguments"] =  [
                    "-f",
                    self["output"],   
                    *self["dependent-files"]
        ]

    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["out-file"])

    @staticmethod
    def configure(otherTasks):

        tasks = []
        fileNames = list(set([task["input-file"] for task in otherTasks]))

        for fileName in fileNames:
            haddConf = {"name": "Hadd_{}".format(fileName.split("/")[-1][:-5]), 
                        "out-file": fileName.split("/")[-1][:-5], 
                        "dir": "/".join(fileName.split("/")[:-1]),
                        "display-name": "Hadd: {}".format(fileName.split("/")[-1][:-5]),
                        "dependencies": [task["name"] for task in otherTasks if task["input-file"] == fileName]
            }

            tasks.append(HaddAppend(haddConf))
        return tasks
