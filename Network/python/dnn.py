from task import Task

import os

class DNN(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "dnn"

        self["arguments"] = [
            "--out-path", self["dir"],
            "--sig-files", *self["sig-files"],
            "--bkg-files", *self["bkg-files"],
            "--opt-param", self["hyper-opt"]
        ]

    def output(self):
        self["output"] = self["dir"] + "/model.pt"

    @staticmethod
    def configure(config, channel, haddTasks, evType, masses, prefix=""): 
        outDir = "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].replace("[C]", channel).replace("[E]", config["era"]).replace("[T]", evType), prefix)
        fileDir = "{}/{}".format(os.environ["CHDIR"], config["dir"].replace("[C]", channel).replace("[E]", config["era"]).replace("[T]", evType))

        task = {
                "name": "DNN_{}_{}".format(channel, evType) + ("_{}".format(prefix) if prefix else ""),
                "dir": outDir,
                "sig-files": ["{}/{}/{}.csv".format(fileDir, sig, sig) for sig in [config["signal"].replace("[MHC]", str(m)) for m in masses]],
                "bkg-files": ["{}/{}/{}.csv".format(fileDir, bkg, bkg) for bkg in config["backgrounds"]],
                "dependencies": [t["name"] for t in haddTasks if evType in t["dir"] and channel in t["dir"]],
                "run-mode": config["run-mode"],
                "hyper-opt": config.get("hyper-opt", "").replace("[C]", channel).replace("[E]", config["era"]).replace("DNN", os.environ["CHDIR"] + "/DNN"), 
                "run-mode": "Condor"
            }
    
        return [DNN(task)]
