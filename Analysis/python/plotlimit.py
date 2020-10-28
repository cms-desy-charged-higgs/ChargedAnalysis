from task import Task
from ROOT import TFile

import os

class PlotLimit(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "plotlimit"

        self["arguments"] = [
                "--charged-masses", *self["charged-masses"],
                "--neutral-masses", *self["neutral-masses"],
                "--channels", *self["channels"],
                "--limit-dir", self["limit-dir"],
                "--out-dirs", self["dir"],
                "--era", self["era"],
                "--x-secs", *self["xSecs"] 
        ]

    def output(self):
        self["output"] = [self["dir"] + "/limit.pdf", self["dir"] + "/limit_by_channel.pdf"]

    @staticmethod
    def configure(config, limitTasks):
        tasks = []

        xSecs = []

        for mHC in config["charged-masses"]:
            for mh in config["neutral-masses"]:
                f = TFile.Open("$CHDIR/Skim/Inclusive/2016/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}/merged/HPlusAndH_ToWHH_ToL4B_{MHC}_{MH}.root".format(MHC=mHC, MH=mh))
                xSecs.append(f.Get("xSec").GetBinContent(1))


        for era in config["era"]:
            outDir = config["dir"].replace("[E]", era).replace("[C]", "")
           
            plotConf = {"name": "PlotLimit_{}".format(era), 
                        "limit-dir": "{}/{}".format(os.environ["CHDIR"], config["dir"].replace("[E]", "").replace("[C]", "")), 
                        "dir":  "{}/CernWebpage/Plots/{}".format(os.environ["CHDIR"], outDir),
                        "display-name": "Plot: Limit", 
                        "dependencies": [t["name"] for t in limitTasks if "Limit" in t["name"] and t["era"] == era],
                        "charged-masses": config["charged-masses"],
                        "neutral-masses": config["neutral-masses"],
                        "channels": ["Combined"] + config["channels"],
                        "era": era,
                        "xSecs": xSecs,
            }

            tasks.append(PlotLimit(plotConf))

        outDir = config["dir"].replace("[E]", "RunII").replace("[C]", "")

        plotConf = {"name": "PlotLimit_{}".format("AllEras"), 
                    "limit-dir": "{}/{}".format(os.environ["CHDIR"], config["dir"].replace("[E]", "").replace("[C]", "")), 
                    "dir":  "{}/CernWebpage/Plots/{}".format(os.environ["CHDIR"], outDir),
                    "display-name": "Plot: Limit", 
                    "dependencies": [t["name"] for t in limitTasks if "Limit" in t["name"] and t["era"] == "RunII"],
                    "charged-masses": config["charged-masses"],
                    "neutral-masses": config["neutral-masses"],
                    "channels": ["Combined"],
                    "era": "RunII",
                    "xSecs": xSecs,
        }
        
        tasks.append(PlotLimit(plotConf))

        return tasks
