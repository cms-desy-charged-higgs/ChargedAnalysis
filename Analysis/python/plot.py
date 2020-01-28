from task import Task

import os

class Plot(Task):
    def __init__(self, config = {}):
        super().__init__(config)

        if not "y-parameter" in self:
            self["y-parameter"] = []

    def run(self):
        self["executable"] = "Plot"

        self["arguments"] = [
                "--hist-dir", self["hist-dir"], 
                "--x-parameters", *self["x-parameters"],
                "--y-parameters", *self["y-parameters"],
                "--channel", self["channel"], 
                "--processes", *self["processes"], 
                "--out-dirs", self["dir"], 
        ]

    def output(self):
        self["output"] = ["{}/{}.pdf".format(self["dir"], x) for x in self["x-parameters"]]

    @staticmethod
    def configure(config, haddTasks, channel):        
        task = {
                "name": "Plot_{}".format(channel), 
                "channel": channel, 
                "hist-dir": os.environ["CHDIR"] + "/Hist/{}/{}".format(config["dir"], config["chan-dir"][channel]), 
                "dir":  os.environ["CHDIR"] + "/CernWebpage/Plots/{}/{}".format(config["dir"], config["chan-dir"][channel]), 
                "display-name": "Plots: {}".format(channel), 
                "x-parameters": config["x-parameters"]["all"] + config["x-parameters"].get(channel, []),
                "y-parameters": config["y-parameters"]["all"] + config["y-parameters"].get(channel, []),
                "dependencies": [t["name"] for t in haddTasks if t["channel"] == channel], 
                "processes": [t["process"] for t in haddTasks if t["channel"] == channel]
        }

        return [Plot(task)]
