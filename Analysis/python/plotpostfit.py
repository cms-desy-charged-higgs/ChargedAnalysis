from task import Task

import os

class PlotPostfit(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "PlotPostfit"

        self["arguments"] = [
                "--limit-dir", self["limit-dir"],
                "--mass", self["mass"],
                "--channels", *self["channels"],
                "--out-dirs", self["dir"],
        ]

    def output(self):
        self["output"] = self["dir"] + "/postfit_{}.pdf".format(self["mass"])

    @staticmethod
    def configure(config, mass):
        tasks = []

        limitConf = {
                    "name": "Postfitplot_{}".format(mass),
                    "limit-dir": "{}/{}".format(os.environ["CHDIR"], config["dir"]), 
                    "dir":  "{}/{}".format(os.environ["CHDIR"], config["dir"]), 
                    "display-name": "Postfit: {}".format(mass), 
                    "dependencies": ["Limit_{}".format(mass)],
                    "channels": config["channels"],
                    "mass": mass,
        }

        tasks.append(PlotPostfit(limitConf))

        return tasks
