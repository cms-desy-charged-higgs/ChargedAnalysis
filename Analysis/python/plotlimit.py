from task import Task

import os

class PlotLimit(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "plotlimit"

        self["arguments"] = [
                "--masses", *self["masses"],
                "--channels", *self["channels"],
                "--limit-dir", self["limit-dir"],
                "--out-dirs", self["dir"]
        ]

    def output(self):
        self["output"] = [self["dir"] + "/limit.pdf", self["dir"] + "/limit_by_channel.pdf"]

    @staticmethod
    def configure(config):
        tasks = []

        plotConf = {"name": "PlotLimit", 
                    "limit-dir": "{}/{}".format(os.environ["CHDIR"], config["dir"]), 
                    "dir":  os.environ["CHDIR"] + "/CernWebpage/Plots/Limits", 
                    "display-name": "Plot: Limit", 
                    "dependencies": ["Limit_{}".format(mass) for mass in config["masses"]],
                    "masses": config["masses"],
                    "channels": config["channels"]
        }

        tasks.append(PlotLimit(plotConf))

        return tasks
