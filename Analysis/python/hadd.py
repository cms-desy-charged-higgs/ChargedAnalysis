from task import Task

import os
from pprint import pprint

class Hadd(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "hadd"
        self["arguments"] = " -f -v 0 {} {}".format(self["output"], " ".join(self["dependent-files"]))

        if self["run-mode"] == "Local":
            os.system("{} {}".format(self["executable"], self["arguments"]))

    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["process"])

    @staticmethod
    def configure(conf, treeTask):
        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1f": "Muon2J1F", "e2j1f": "Ele2J1F", "mu2f": "Muon2F", "e2f": "Ele2F"}

        tasks = []

        for channel in conf.keys():
            for process in conf[channel]["processes"]:
                outDir = os.environ["CHDIR"] + "/Hist/{}/{}".format(conf[channel]["dir"], chanToDir[channel])
                
                haddConf = {"name": "Hadd_{}_{}".format(process, channel),  
                            "dir": outDir,
                            "process": process,
                            "channel": channel,
                            "display-name": "Hadd: {} ({})".format(process, channel),
                            "dependencies": [t["name"] for t in treeTask if t["dir"] == outDir and t["process"] == process]
                }

                tasks.append(Hadd(haddConf))
        return tasks

        
