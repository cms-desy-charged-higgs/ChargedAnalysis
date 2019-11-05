from task import Task

import os

class HaddPlot(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "hadd"
        self["arguments"] =  [
                                "-f",
                                "-v", 
                                "0",
                                self["output"],   
        ] + self["dependent-files"]

        super()._run()

    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["process"])

    @staticmethod
    def configure(conf, treeTasks, channel):
        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1f": "Muon2J1F", "e2j1f": "Ele2J1F", "mu2f": "Muon2F", "e2f": "Ele2F"}

        tasks = []

        for process in conf[channel]["processes"]:
            outDir = os.environ["CHDIR"] + "/Hist/{}/{}".format(conf[channel]["dir"], chanToDir[channel])
            histDir = os.environ["CHDIR"] + "/Tmp/Hist/{}/{}".format(conf[channel]["dir"], chanToDir[channel])
                
            haddConf = {"name": "Hadd_{}_{}".format(process, channel),  
                        "dir": outDir,
                        "process": process,
                        "channel": channel,
                        "display-name": "Hadd: {} ({})".format(process, channel),
                        "dependencies": [t["name"] for t in treeTasks if t["dir"] == histDir and t["process"] == process]
            }

            tasks.append(HaddPlot(haddConf))
        return tasks


class HaddAppend(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "hadd"
        self["arguments"] =  [
                                "-f",
                                "-v", 
                                "0",
                                self["output"],   
        ] + self["dependent-files"]

        super()._run()

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

        
