from task import Task

import os

class FileSkim(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "FileSkim"

        self["arguments"] = [
                self["input-file"], 
                self["output"],
                "{}".format(" ".join(self["exclude"])),
        ]

        super()._run()

    def output(self):
        self["output"] = "{}/{}_skimmed.root".format(self["dir"], self["input-file"].split("/")[-1][:-5])

    @staticmethod
    def configure(appendTask, channels):
        tasks = []

        fileNames = list(set([task["input-file"] for task in appendTask]))

        for fileName in fileNames:
            config = {"name": "FileSkim_{}".format(fileName.split("/")[-1][:-5]), 
                        "dir": os.environ["CHDIR"] + "/Tmp/FileSkim", 
                        "input-file": fileName,
                        "exclude": channels,
                        "display-name": "FileSkim: {}".format(fileName.split("/")[-1][:-5])
            }

            tasks.append(FileSkim(config))
        return tasks

