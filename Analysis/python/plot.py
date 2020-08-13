from task import Task

import os

class Plot(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "plot"

        self["arguments"] = [
                "--hist-dir", self["hist-dir"], 
                "--channel", self["channel"], 
                "--processes", *self["processes"], 
                "--out-dirs", self["dir"], self["web-dir"] 
        ]

    def output(self):
        self["output"] = ["{}/{}.pdf".format(self["dir"], "kappa")]

    @staticmethod
    def configure(config, haddTasks, channel):        
        task = {
                "name": "Plot_{}".format(channel), 
                "channel": channel, 
                "hist-dir": os.environ["CHDIR"] + "/{}".format(config["dir"].replace("[C]", channel).replace("[E]", config["era"])), 
                "dir":  os.environ["CHDIR"] + "/Plots/{}".format(config["dir"].replace("[C]", channel).replace("[E]", config["era"])), 
                "web-dir": os.environ["CHDIR"] + "/CernWebpage/Plots/{}".format(config["dir"].replace("[C]", channel).replace("[E]", config["era"])), 
                "display-name": "Plots: {}".format(channel), 
                "dependencies": [t["name"] for t in haddTasks if t["channel"] == channel], 
                "processes": [t["process"] for t in haddTasks if t["channel"] == channel]
        }

        return [Plot(task)]
