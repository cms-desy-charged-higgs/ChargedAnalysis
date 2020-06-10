from task import Task

import os

class Datacard(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "writecard"

        self["arguments"] = [
                "--backgrounds", *self["backgrounds"],
                "--signal", self["signal"],
                "--data", self["data"],
                "--channel", self["channel"],
                "--out-dir", self["dir"],
                "--use-asimov" if self["data"] == "" else "",
                "--hist-dir", self["hist-dir"], 
                "--discriminant", self["discriminant"], 
        ]

    def output(self):
        self["output"] = [self["dir"] + "/datacard.txt", self["dir"] + "/shapes.root"]

    @staticmethod
    def configure(config, channel, mass, haddTasks):
        tasks = []

        cardConf = {
                "name": "Datacard_{}_{}".format(mass, channel), 
                "dir":  "{}/{}/{}".format(os.environ["CHDIR"], config["dir"], channel), 
                "display-name": "Datacard: {} ({})".format(mass, channel), 
                "discriminant": config["discriminant"],
                "dependencies": [t["name"] for t in haddTasks if channel in t["channel"]], 
                "backgrounds": config["backgrounds"],
                "signal": config["signal"],
                "hist-dir": "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].replace(str(mass), ""),  channel),
                "channel": channel,
                "data": config["data"].get("channel", "")
        }

        tasks.append(Datacard(cardConf))

        return tasks
        
