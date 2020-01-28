from task import Task

import os

class MergeCSV(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "MergeCSV"

        self["arguments"] = [
                "--labels", *self["labels"],
                "--dir", self["dir"],
                "--out-name", self["out-name"],
                "--sort", self["sort"],
        ]

    def output(self):
        self["output"] = self["dir"] + "/{}.csv".format(self["out-name"])

    @staticmethod
    def configure(config, otherTask, prefix=""):
        task = {
                "name": "MergeCSV" + ("_{}".format(prefix) if prefix else ""),
                "labels": [config["sort-by"]] + list(config["hyper-parameter"].keys()), 
                "dir": "{}/{}/HyperTuning".format(os.environ["CHDIR"], config["dir"].format("")),
                "out-name": "parameter",
                "sort": config["sort-by"],
                "dependencies": [t["name"] for t in otherTask]
        }

        return [MergeCSV(task)]
