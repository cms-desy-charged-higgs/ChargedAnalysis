from task import Task

import os
import yaml

class TreeSlim(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "treeslim"

        self["arguments"] = [
                "--input-name", self["input-name"],
                "--input-channel", self["input-channel"],
                "--out-name", self["output"],
                "--out-channel", self["out-channel"],
                "--cuts", *self["cuts"],
                "--dCache", self["dCache"],
        ]
        
    def output(self):
        self["output"] = "{}/{}".format(self["dir"], self["out-name"])

    @staticmethod
    def configure(config, prefix=""):
        tasks = []

        for (outChannel, inChannel) in config["channels"].items():
            for syst in config["shape-systs"].get("all", []) + config["shape-systs"].get(inChannel, []):
                for shift in ["Up", "Down"]:
                    ##Construct systname and skim dir
                    systName = "" if syst == "" else "{}{}".format(syst, shift)
                    skimDir = "{}/{}".format(os.environ["CHDIR"], config["skim-dir"].replace("[E]", config["era"]))

                    ##If nominal skip Down loop
                    if(syst == "" and shift == "Down"):
                        continue

                    for fileName in os.listdir(skimDir):
                        ##Skip if data and not nominal
                        if ("Electron" in fileName or "Muon" in fileName or "Gamma" in fileName) and syst != "":
                            continue

                        outDir = "{}/{}/{}/merged/{}".format(os.environ["CHDIR"], config["out-dir"].replace("[E]", config["era"]).replace("[C]", outChannel), fileName, systName)

                        inFile = "{}/{}/merged/{}/{}.root".format(skimDir, fileName, systName, fileName)

                        if not os.path.exists(inFile):
                            raise FileNotFoundError("File not found: " +  inFile)

                        ##Configuration for treeread Task
                        task = {
                            "name": "{}_{}{}".format(fileName, outChannel, systName) + ("_{}".format(prefix) if prefix else ""), 
                            "display-name": "Slim: {}".format(outChannel),
                            "input-name": inFile,
                            "input-channel": inChannel,
                            "out-channel": outChannel, 
                            "cuts": config["cuts"].get(outChannel, []),
                            "out-name": "{}.root".format(fileName), 
                            "dir": outDir,
                            "dCache": "{}/{}/merged/{}".format(config["dCache"].replace("[E]", config["era"]).replace("[C]", outChannel), fileName, systName) if "dCache" in config else "", 
                            "run-mode": config["run-mode"],
                        }

                        tasks.append(TreeSlim(task))

        return tasks
