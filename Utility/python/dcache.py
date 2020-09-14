from task import Task

import os
import yaml

class DCache(Task):
    def __init__(self, config = {}):
        super().__init__(config)
        
        self.cleanDir = True

    def run(self):
        self["executable"] = "sendToDCache"

        self["arguments"] = [
                "--input-file", self["input-file"],
                "--dcache-path", self["dcache-path"], 
                "--relative-path", self["relative-path"],
        ]
        
    def output(self):
        self["output"] = [self["input-file"].replace(os.environ["CHDIR"], self["dcache-path"])]

    @staticmethod
    def configure(config, dependentTasks, prefix = ""):
        tasks = []

        for index, t in enumerate(dependentTasks): 
            task = {
                    "name": "toDache_{}".format(index), 
                    "input-file": "{}/{}".format(t["dir"], t["out-name"]),
                    "display-name": "SendtoDache",
                    "out-name": t["out-name"], 
                    "dir": "/tmp/tmp_{}".format(id(t)),
                    "dependencies": [t["name"]],
                    "dcache-path": "/pnfs/desy.de/cms/tier2/store/user/dbrunner/",
                    "relative-path": t["dir"].replace(os.environ["CHDIR"], "")[1:]
            }

            tasks.append(DCache(task))            
            
        return tasks
