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
                "--out-dirs", self["dir"], self["web-dir"],
                "--era", self["era"]
        ]

    def output(self):
        self["output"] = ["{}/{}.pdf".format(self["dir"], "kappa")]

    @staticmethod
    def configure(config, haddTasks, channel, era):        
        task = {
                "name": "Plot_{}_{}".format(channel, era), 
                "channel": channel, 
                "hist-dir": os.environ["CHDIR"] + "/{}".format(config["dir"].replace("[C]", channel).replace("[E]", era)), 
                "dir":  os.environ["CHDIR"] + "/Plots/{}".format(config["dir"].replace("[C]", channel).replace("[E]", era)), 
                "web-dir": os.environ["CHDIR"] + "/CernWebpage/Plots/{}".format(config["dir"].replace("[C]", channel).replace("[E]", era)), 
                "display-name": "Plots: {}/{}".format(channel, era), 
                "dependencies": [t["name"] for t in haddTasks if t["channel"] == channel], 
                "processes": list(set([t["process"] for t in haddTasks if t["channel"] == channel])),
                "era": era
        }

        return [Plot(task)]
