from task import Task

import os

class BDT(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "boostedDT"

        self["arguments"] = [
                "--sep-type", self["sep-type"],
                "--parameters", *self["parameters"],
                "--tree-dir", self["tree-dir"],
                "--result-dir", self["dir"],
                "--signal", self["signal"],
                "--backgrounds", *self["backgrounds"],
                "--masses", *self["masses"],
                "--optimize" if self["optimize"] != -1 else "",
        ]

        if self["optimize"] == -1:
            self["arguments"] += [
                "--n-trees", self["n-trees"],
                "--min-node-size", self["min-node-size"],
                "--lr", self["lr"], 
                "--n-cuts", self["n-cuts"],
                "--tree-depth", self["tree-depth"],
                "--drop-out", self["drop-out"],
            ]

    def output(self):
        self["output"] = self["dir"] + "/BDT.root"

    @staticmethod
    def configure(config, channel, haddTasks, evType, prefix=""):
        task = {
                "name": "BDT_{}_{}".format(channel, evType) + ("_{}".format(prefix) if prefix else ""),
                "dir": "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].format(evType), config["chan-dir"][channel]),
                "signal": config["signal"],
                "backgrounds": config["backgrounds"],
                "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
                "tree-dir": "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].format(evType), config["chan-dir"][channel]), 
                "masses": config["masses"],
                "dependencies": [t["name"] for t in haddTasks if evType in t["dir"] and config["chan-dir"][channel] in t["dir"]],
                "run-mode": config["run-mode"],
                "optimize": config.get("optimize", -1),
                "sep-type": config["sep-type"],
            }

        task.update(config["hyper-parameter"])
    
        return [BDT(task)]
                    
        
