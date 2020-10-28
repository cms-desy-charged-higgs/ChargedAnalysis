from task import Task

import os

class Limit(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "limit"

        self["arguments"] = [
                "--out-dir", self["dir"],
                "--card-dir", self["card-dir"],   
        ]

    def output(self):
        self["output"] = self["dir"] + "/limit.root"

    @staticmethod
    def configure(config, channel, era, cardTasks):
        tasks = []

        for mHC in config["charged-masses"]:
            for mh in config["neutral-masses"]:
                cardDir = "{}/{}/HPlus{}_h{}".format(os.environ["CHDIR"], config["dir"].replace("[E]", era).replace("[C]", channel), mHC, mh)

                limitConf = {
                            "name": "Limit_{}_{}_{}_{}".format(channel, era, mHC, mh), 
                            "dir":  "{}/Limit".format(cardDir),
                            "card-dir": cardDir,
                            "display-name": "Limit {}/{} ({}/{})".format(channel, era, mHC, mh), 
                            "dependencies": [t["name"] for t in cardTasks if t["dir"] == cardDir],
                            "channel": channel,
                            "era": era
                }

                tasks.append(Limit(limitConf))

        return tasks
