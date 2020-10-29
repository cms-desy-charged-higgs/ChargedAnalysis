from task import Task

import os

class MergeCSV(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "mergeCSV"

        self["arguments"] = [
                "--input", *self["dependent-files"],
                "--output", self["output"],
        ]

    def output(self):
        self["output"] = "{}/{}.csv".format(self["dir"], self["process"])

    @staticmethod
    def configure(config, treeTasks, channel, era, prefix=""):
        tasks = []

        for process in config["processes"] + config["data"].get(channel, []):
            outDir = os.environ["CHDIR"] + "/{}/{}/{}".format(config["dir"], channel, process)
                
            task = {
                    "name": "MergeCSV_{}_{}_{}".format(process, channel, era) + ("_{}".format(prefix) if prefix else ""),  
                    "dir": outDir,
                    "channel": channel,
                    "process": process,
                    "era": era,
                    "display-name": "Merge CSV: {} ({}/{})".format(process, channel, era),
                    "dependencies": [t["name"] for t in treeTasks if process == t["process"] and channel == t["channel"] and era == t["era"]]
            }

            tasks.append(MergeCSV(task))
        return tasks
