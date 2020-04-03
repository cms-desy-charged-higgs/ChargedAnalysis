from task import Task
import utils

import os

class FileSkim(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "fileskim"

        self["arguments"] = [
                "--old-file", self["input-file"], 
                "--new-file", self["output"],
                "--skip-objs", *self["exclude"]
        ]

    def output(self):
        self["output"] = "{}/{}_skimmed.root".format(self["dir"], self["input-file"].split("/")[-1][:-5])

    @staticmethod
    def configure(config, appendTask):
        tasks = []

        fileNames = list(set([task["input-file"] for task in appendTask]))

        for fileName in fileNames:
            task = {
                    "name": "FileSkim_{}".format(fileName.split("/")[-1][:-5]), 
                    "dir": "{}/{}/FileSkim/{}".format(os.environ["CHDIR"], config["dir"], fileName), 
                    "input-file": fileName,
                    "exclude": config["channels"],
                    "display-name": "FileSkim: {}".format(fileName.split("/")[-1][:-5])
            }

            tasks.append(FileSkim(task))

        return tasks

