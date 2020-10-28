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
                "--systematics", *self["systematics"]
        ]

    def output(self):
        self["output"] = [self["dir"] + "/datacard.txt", self["dir"] + "/shapes.root"]

    @staticmethod
    def configure(config, channel, era, haddTasks):
        tasks = []

        for mHC in config["charged-masses"]:
            for mh in config["neutral-masses"]:
                outDir = "{}/{}".format(os.environ["CHDIR"], config["dir"].replace("[E]", era).replace("[C]", channel))
        
                cardConf = {
                        "name": "Datacard_{}_{}_{}_{}".format(str(mHC), str(mh), channel, era), 
                        "dir": "{}/HPlus{}_h{}".format(outDir, mHC, mh), 
                        "display-name": "Datacard: {}/{} ({}/{})".format(str(mHC), str(mh), channel, era), 
                        "discriminant": config["discriminant"].replace("[MHC]", str(mHC)),
                        "dependencies": [t["name"] for t in haddTasks if channel in t["name"] and era in t["name"]], 
                        "backgrounds": config["backgrounds"],
                        "signal": config["signal"].replace("[MHC]", str(mHC)).replace("[MH]", str(mh)),
                        "hist-dir": outDir,
                        "channel": channel,
                        "data": config["data"].get("channel", ""),
                        "era": era,
                        "systematics": ['""'] + config["shape-systs"].get("all", [])[1:] + config["shape-systs"].get(channel, []) + config["scale-systs"].get("all", [])[1:] + config["scale-systs"].get(channel, []),
                }

                tasks.append(Datacard(cardConf))

        return tasks
        
