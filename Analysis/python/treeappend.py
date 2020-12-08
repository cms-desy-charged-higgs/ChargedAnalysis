from task import Task
import utils

import os
import yaml

class TreeAppend(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "treeappend"

        self["arguments"] = [
                "--file-name", "{}/{}".format(self["dir"], self["out-name"]), 
                "--tree-name", self["channel"],
                "--functions", *self["functions"],
                "--era", self["era"]
        ]

    def output(self):
        self["output"] = "{}/{}".format(self["dir"], self["out-name"])
   
    @staticmethod
    def configure(config, channel, era):
        tasks = []

        skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("[C]", channel).replace("[E]", era))

        for fileName in os.listdir(skimDir):
            for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(channel, []):
                for shift in ["Up", "Down"]:
                    ##Skip Down for nominal case
                    if syst == "" and shift == "Down":
                        continue
                    systName = "{}{}".format(syst, shift) if syst != "" else ""

                    if "Run2" in fileName and syst != "":
                        continue

                    ##Configuration for treeread Task
                    task = {
                        "name": "Append_{}_{}__{}_{}".format(channel, fileName, era, systName), 
                        "display-name": "Append: {} ".format(channel),
                        "dir":  "{}/{}/merged/{}".format(skimDir, fileName, systName),
                        "out-name": "{}.root".format(fileName),
                        "channel": channel,
                        "functions": config["functions"].get("all", []) + config["functions"].get(channel, []),
                        "run-mode": config["run-mode"],
                        "era": era,
                    }

                    tasks.append(TreeAppend(task))

        return tasks       
