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

        for fileName in os.listdir("{}/{}".format(os.environ["CHDIR"], config["skim-dir"])):
            for syst in [""] + config["shape-systs"]:
                for shift in ["Up", "Down"]:
                    if(syst == "" and shift == "Down"):
                        continue

                    systName = "" if syst == "" else "_{}{}".format(syst, shift)
                    systDir = "" if syst == "" else "Syst/{}/".format(systName.replace("_", ""))

                    inFile = "{}/{}/{}/merged/{}{}.root".format(os.environ["CHDIR"], config["skim-dir"], fileName, fileName, systName)
                    if not os.path.isfile(inFile):
                        continue

                    for (outChannel, inChannel) in config["channels"].items():
                        outDir = "{}/{}/{}/merged".format(os.environ["CHDIR"], config["out-dir"].replace("@", outChannel), fileName)

                        ##Configuration for treeread Task
                        task = {
                            "name": "{}_{}{}".format(fileName, outChannel, systName) + ("_{}".format(prefix) if prefix else ""), 
                            "display-name": "Slim: {}".format(outChannel),
                            "input-name": inFile,
                            "input-channel": inChannel,
                            "out-channel": outChannel, 
                            "cuts": config["cuts"].get(outChannel, []),
                            "out-name": "{}{}.root".format(fileName, systName), 
                            "dir": "{}/{}/{}/merged/{}".format(os.environ["CHDIR"], config["out-dir"].replace("@", outChannel), fileName, systDir),
                            "dCache": "{}/{}/merged/{}".format(config["dCache"].replace("@", outChannel), fileName, systDir) if "dCache" in config else "", 
                            "run-mode": config["run-mode"],
                        }

                        tasks.append(TreeSlim(task))

        return tasks
