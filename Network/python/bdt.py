from task import Task

import os

class BDT(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "BDT"

        self["arguments"] = [
                "--n-trees", self["n-trees"],
                "--min-node-size", self["min-node-size"],
                "--lr", self["lr"], 
                "--n-cuts", self["n-cuts"],
                "--tree-depth", self["tree-depth"],
                "--drop-out", self["drop-out"],
                "--sep-type", self["sep-type"],
    
                "--x-parameters", self["x-parameters"],
                "--tree-dir", *self["tree-dir"],
                "--result-dir", self["dir"],
                "--signal", self["signal"],
                "--backgrounds", *self["backgrounds"],
                "--masses", *self["masses"],
                "--event-type", self["event-type"],
        ]

        return super()._run()

    def output(self):
        self["output"] = self["dir"] + "/BDT.root"

    @staticmethod
    def configure(config, channel, evType):
        task = {
                "name": "BDT_{}_{}".format(channel, evType)
                "dir": "{}/{}".format(os.environ["CHDIR"], config["dir"])
                "signal": config["signal"]
                "backgrounds": config["backgrounds"]
                "x-parameters": config["x-parameters"].get("all", []) + config["x-parameters"].get(channel, []),
                "tree-dir": "{}/Tree/{}/{}".format(os.environ["CHDIR"], config["dir"], config["channel-dirs"][channel]), 
                "masses": config["masses"],
                "event-type": evType,       
            }

        task.update(config["hyper-parameter"])

    return [BDT(task)]
                    
        
