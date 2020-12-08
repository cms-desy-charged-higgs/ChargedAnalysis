from task import Task

import os
import yaml

class HTagger(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "htag"

        self["arguments"] = [
            "--out-dir", self["dir"],
            "--channels", *self["channels"],   
            "--cuts", *self["cuts"],
            "--sig-files", *self["sig-files"],
            "--bkg-files", *self["bkg-files"],
            "--opt-param", self["hyper-opt"],
        ]

    def output(self):
        self["output"] = self["dir"] + "/htagger.pt"

    @staticmethod
    def configure(config, evType, era, prefix=""):
        tasks = []

        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        outDir = "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].replace("[E]", era).replace("[T]", evType), prefix)
        backgrounds, signals = [], []
    
        for channel in config["channels"]:
            skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("[C]", channel).replace("[E]", era))

            for d in os.listdir(skimDir):
                for process in config["backgrounds"]:
                    for processName in processDic[process]:
                        if d.startswith(processName):
                            backgrounds.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

                for process in config["signals"]:
                    for processName in processDic[process]:
                        if d.startswith(processName):
                            signals.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

        task = {
            "name": "HTagger_{}_{}".format(evType, era) + ("_{}".format(prefix) if prefix else ""),
            "display-name": "HTagger ({})".format(era),
            "dir": outDir,
            "sig-files": signals,
            "bkg-files": backgrounds,
            "run-mode": config["run-mode"],
            "hyper-opt": config.get("hyper-opt", "").replace("DNN", os.environ["CHDIR"] + "/DNN"), 
            "channels": config["channels"],
            "cuts": [c + "/{}".format(chan) for chan in config["channels"] for c in config["cuts"].get("all", []) + config["cuts"].get(chan, [])],
        }

        tasks.append(HTagger(task))
    
        return tasks
