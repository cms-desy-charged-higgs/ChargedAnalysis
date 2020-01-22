from task import Task

import os

class Limit(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "Limit"

        self["arguments"] = [
                "--mass", self["mass"],
                "--limit-dir", self["dir"],   
                "--channels", *self["channels"],
        ]

        return super()._run()

    def output(self):
        self["output"] = self["dir"] + "/limit.root"

    @staticmethod
    def configure(config, mass, cardTasks):
        tasks = []

        limitConf = {"name": "Limit_{}".format(mass), 
                    "dir":  "{}/{}".format(os.environ["CHDIR"], config[list(config.keys())[1:][0]]["dir"]), 
                    "display-name": "Limit: {}".format(mass), 
                    "dependencies": [t["name"] for t in cardTasks if str(mass) in t["dir"]],
                    "channels": list(config.keys())[1:],
                    "mass": str(mass),
        }

        tasks.append(Limit(limitConf))

        return tasks
