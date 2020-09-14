from task import Task
from ROOT import TFile

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
                "--event-start", self["event-start"],
                "--event-end", self["event-end"],
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

                        inFile = "{}/{}/merged/{}/{}.root".format(skimDir, fileName, systName, fileName)

                        if not os.path.exists(inFile):
                            raise FileNotFoundError("File not found: " +  inFile)

                        f = TFile.Open(inFile)
                        t = f.Get(inChannel)   
                        nEvents = t .GetEntries()

                        eventRanges = [[i if i == 0 else i+1, i+config["n-events"] if i+config["n-events"] <= nEvents else nEvents] for i in range(0, nEvents, config["n-events"])]

                        ##Configuration for treeread Task   
                        for index, (start, end) in enumerate(eventRanges):
                            outDir = "{}/{}/{}/unmerged/{}/{}".format(os.environ["CHDIR"], config["out-dir"].replace("[E]", config["era"]).replace("[C]", outChannel), fileName, systName, index)

                            ##Configuration for treeread Task
                            task = {
                                "name": "{}_{}{}_{}".format(fileName, outChannel, systName, index) + ("_{}".format(prefix) if prefix else ""), 
                                "display-name": "Slim: {}".format(outChannel),
                                "input-name": inFile,
                                "input-channel": inChannel,
                                "out-channel": outChannel, 
                                "cuts": config["cuts"].get(outChannel, []),
                                "out-name": "{}.root".format(fileName), 
                                "dir": outDir,
                                "run-mode": config["run-mode"],
                                "event-start": start,
                                "event-end": end,
                                "systematic": systName, 
                            }

                            tasks.append(TreeSlim(task))

        return tasks
