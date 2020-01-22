from task import Task

import os

class PlotLimit(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "PlotLimit"

        self["arguments"] = [
                "--masses", self["masses"],
                "--limit-dir", self["limit-dir"],
                "--out-dirs", self["dir"]
        ]

        return super()._run()

    def output(self):
        self["output"] = self["dir"] + "/limit.pdf"

    @staticmethod
    def configure(config):
        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1f": "Muon2J1F", "e2j1f": "Ele2J1F", "mu2f": "Muon2F", "e2f": "Ele2F"}
        
        tasks = []

        plotConf = {"name": "PlotLimit", 
                    "limit-dir": "{}/{}".format(os.environ["CHDIR"], config[list(config.keys())[1:][0]]["dir"][:-1]), 
                    "dir":  os.environ["CHDIR"] + "/CernWebpage/Plots/Limits", 
                    "display-name": "Plot: Limit", 
                    "dependencies": ["Limit_{}".format(str(mass)) for mass in config["masses"]],
                    "masses": [str(mass) for mass in config["masses"]],
        }

        tasks.append(PlotLimit(plotConf))

        return tasks
