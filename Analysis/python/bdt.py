from task import Task

import os

class BDTTask(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "BDT"

        self["arguments"] = [
                "--n-trees", self["n-trees"],
                "--min-node-size", self["min-node-size"]
                "--lr", self["lr"], 
                "--n-cuts", self["n-cuts"],
                "--tree-depth", self["tree-depth"],
                "--drop-out", self["drop-out"],
                "--sep-type", self["sep-type"],
    
                "--x-parameters", self["--x-parameters"], 
                "--tree-dir", *self["tree-dir"],
                "--result-dir", self["result-dir"],
                "--signals", *self["signals"],
                "--backgrounds", *self["backgrounds"],
                "--event-type", self["event-type"]
        ]

        return super()._run()

        bdt = BDT(1560, 1.1005293101958564, 0.49858931820896, 34, 5, 11, "CrossEntropy")
        bdt.Train(self._stdDir["x-parameter"], self._stdDir["tree-dir"], self._stdDir["dir"], self._stdDir["signal"], self._stdDir["background"], self._stdDir["event-type"]) 

    def output(self):
        self["output"] = self["dir"] + "/BDT.root"
