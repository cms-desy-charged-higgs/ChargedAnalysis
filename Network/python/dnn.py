from task import Task

import os
import yaml

class DNN(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "dnn"

        self["arguments"] = [
            "--out-path", self["dir"],
            "--channel", self["channel"],   
            "--cuts", *self["cuts"],
            "--parameters", *self["parameters"],
            "--sig-files", *self["sig-files"],
            "--bkg-files", *self["bkg-files"],
            "--masses", *self["masses"],
            "--opt-param", self["hyper-opt"],
            "--clean-jets", self["clean-jets"]
        ]

        with open(self["dir"] + "/parameter.txt", "w") as param:
            for parameter in self["parameters"]:
                param.write(parameter + "\n")

    def output(self):
        self["output"] = self["dir"] + "/model.pt"

    @staticmethod
    def configure(config, evType, masses, era, prefix=""):
        tasks = []

        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        for channel in config["channels"]: 
            outDir = "{}/{}/{}".format(os.environ["CHDIR"], config["dir"].replace("[C]", channel).replace("[E]", era).replace("[T]", evType), prefix)

            skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("[C]", channel).replace("[E]", era))
            backgrounds, signals = [], []

            for d in sorted(os.listdir(skimDir)):
                for process in config["backgrounds"]:
                    for processName in processDic[process]:
                        if d.startswith(processName):
                            backgrounds.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

                for process in [config["signal"].replace("[MHC]", str(mass)) for mass in masses]:
                    for processName in processDic[process]:
                        if d.startswith(processName):
                            signals.append("{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = d))

            task = {
                    "name": "DNN_{}_{}_{}".format(channel, evType, era) + ("_{}".format(prefix) if prefix else ""),
                    "display-name": "DNN ({}/{})".format(channel, era),
                    "dir": outDir,
                    "sig-files": signals,
                    "bkg-files": backgrounds,
                    "run-mode": config["run-mode"],
                    "hyper-opt": config.get("hyper-opt", "").replace("[C]", channel).replace("DNN", os.environ["CHDIR"] + "/DNN"), 
                    "run-mode": config["run-mode"],
                    "channel": channel,
                    "parameters": config["parameters"].get("all", []) + config["parameters"].get(channel, []),
                    "cuts": config["cuts"].get("all", []) + config["cuts"].get(channel, []),
                    "masses": masses,
                    "clean-jets": config["clean-jets"].get(channel, ""),
                }

            tasks.append(DNN(task))
    
        return tasks
