from task import Task
import utils

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
                "--x-parameters", *self["x-parameter"],
                "--y-parameters", *self["y-parameter"],
                "--channel", self["channel"], 
                "--processes", *self["processes"], 
                "--out-dirs", self["dir"], 
        ]

        return super()._run()

    def output(self):
        self["output"] = ["{}/{}.pdf".format(self["dir"], x) for x in self["x-parameter"]]

    @staticmethod
    def configure(conf, haddTasks, channel):        
        tasks = []

        plotConf = {"name": "Plot_{}".format(channel), 
                    "channel": channel, 
                    "hist-dir": os.environ["CHDIR"] + "/Hist/{}/{}".format(conf[channel]["dir"], utils.ChannelToDir(channel)), 
                    "dir":  os.environ["CHDIR"] + "/CernWebpage/Plots/{}/{}".format(conf[channel]["dir"], utils.ChannelToDir(channel)), 
                    "display-name": "Plots: {}".format(channel), 
                    "x-parameter": conf[channel]["x-parameter"], 
                    "dependencies": [t["name"] for t in haddTasks if t["channel"] == channel], 
                    "processes": [t["process"] for t in haddTasks if t["channel"] == channel]
        }

        if "y-parameter" in conf[channel]:
            plotConf["y-parameter"] = conf[channel]["y-parameter"]


        tasks.append(Plot(plotConf))

        return tasks
