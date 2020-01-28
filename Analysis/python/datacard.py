from task import Task

import os

class Datacard(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "WriteCard"

        self["arguments"] = [
                "--backgrounds", *self["backgrounds"],
                "--signal", self["signal"],
                "--data", self["data"],
                "--channel", self["channel"],
                "out-dir", self["dir"],
                "--use-asimov" if not True in ["Single" in proc for proc in self["processes"]] else "",
                "--hist-dir", self["hist-dir"], 
                "--discriminant", self["discriminant"], 
        ]

    def output(self):
        self["output"] = [self["dir"] + "/datacard.txt", self["dir"] + "/shapes.root"]

    @staticmethod
    def configure(config, channel, mass, haddTasks):
        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1f": "Muon2J1F", "e2j1f": "Ele2J1F", "mu2f": "Muon2F", "e2f": "Ele2F"}
        
        tasks = []

        cardConf = {"name": "Datacard_{}_{}".format(mass, channel), 
                    "dir":  "{}/{}/{}".format(os.environ["CHDIR"], config[channel]["dir"], chanToDir[channel]), 
                    "display-name": "Datacard: {} ({})".format(mass, channel), 
                    "discriminant": config[channel]["x-parameter"][0],
                    "dependencies": [t["name"] for t in haddTasks if str(mass) in t["dir"] and t["channel"] == channel], 
                    "backgrounds": config[channel]["backgrounds"],
                    "signal": config[channel]["signal"],
                    "data": config[channel]["data"],
                    "hist-dir": "{}/Hist/{}".format(os.environ["CHDIR"], config[channel]["dir"]),
                    "channel": channel,
        }


        tasks.append(Datacard(cardConf))

        return tasks
        
