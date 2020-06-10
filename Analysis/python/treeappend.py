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
                "--file-name", self["input-file"], 
                "--tree-name", self["channel"],
                "--functions", *self["functions"],
                "--dCache", self["dCache"],
        ]

    def output(self):
        self["output"] = self["input-file"]
   
    @staticmethod
    def configure(config, channel):
        tasks = []

        for fileName in os.listdir("{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("@",  channel))):
            inFile = "{}/{}/{}/merged/{}.root".format(os.environ["CHDIR"], config["skim-dir"].replace("@",  channel), fileName, fileName)

            ##Configuration for treeread Task
            task = {
                "name": "Append_{}_{}".format(channel, fileName), 
                "display-name": "Append: {} ".format(channel),
                "dir":  "{}/{}/{}/merged".format(os.environ["CHDIR"], config["skim-dir"].replace("@",  channel), fileName),
                "input-file": inFile,
                "channel": channel,
                "functions": config["functions"].get("all", []) + config["functions"].get(channel, []),
                "run-mode": config["run-mode"], 
                "dCache": "{}/{}/merged".format(config["dCache"].replace("@", channel), fileName) if "dCache" in config else "", 
            }

            tasks.append(TreeAppend(task))

        return tasks       
