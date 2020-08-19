from task import Task

import os
import copy

class MergeSkim(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "mergeskim"
        self["arguments"] =  [
                    "--out-file", self["output"],   
                    "--input-files", *self["input-files"],
                    "--optimize" if "optimize" in self else "",
                    "--dCache", self["dCache"],
        ]

    def output(self):
        self["output"] = self["output-file"]

    @staticmethod
    def configure(config):
        tasks = []

        skimDir = config["skim-dir"].replace("[E]", config["era"])

        ##Loop over all directories in the skim dir
        for d in os.listdir("{}/{}".format(os.environ["CHDIR"], skimDir)):
            mergeFiles = {}

            ##Loop over all systematics
            for syst in config["shape-systs"]["all"]:
                counter = 0

                ##Skip if data and not nominal
                if ("Electron" in d or "Muon" in d or "Gamma" in d) and syst != "":
                    continue

                ##Loop over shifts
                for shift in ["Up", "Down"]:
                    ##Skip Down for nominal case
                    if syst == "" and shift == "Down":
                        continue

                    systName = "{}{}".format(syst, shift) if syst != "" else ""

                    ##Open file with xrootd paths to output files
                    with open("{}/{}/{}/outputFiles.txt".format(os.environ["CHDIR"], skimDir, d)) as fileList:
                        files = []
                        nFilesMerge = 30
                        count = 0

                        nFiles = len(list(fileList))
                        fileList.seek(0)

                        for idx, f in enumerate(fileList):
                            fileName = ""

                            if syst != "":
                                if systName in f:
                                    fileName = f.replace("\n", "")
                            else:
                                if "Up" not in f and "Down" not in f:
                                    fileName = f.replace("\n", "")

                            if(fileName != ""):
                                files.append(fileName)

                            if (len(files) > nFilesMerge or idx == nFiles - 1) and len(files) != 0:
                                outDir =  "{}/{}/{}/tmp/{}/{}".format(os.environ["CHDIR"], skimDir, d, systName, count)

                                task = {
                                    "name": "MergeSkim_{}_{}".format(d, count) + ("_{}".format(systName) if syst != "" else ""),
                                    "dir": outDir, 
                                    "output-file": "{}/{}.root".format(outDir, d),  
                                    "input-files": copy.deepcopy(files),                  
                                    "optimize": True,
                                    "run-mode": config["run-mode"],
                                    "dCache": "",
                                }
  
                                tasks.append(MergeSkim(task))
                                files.clear()   
                                count += 1

                        outDir =  "{}/{}/{}/merged/{}".format(os.environ["CHDIR"], skimDir, d, systName)

                        task = {
                            "name": "MergeSkim_{}".format(d) + ("_{}".format(systName) if syst != "" else ""),
                            "dir": outDir, 
                            "dCache": "{}/merged/{}".format(config["dCache"].replace("[E]", config["era"]).replace("[P]", d), systName),
                            "output-file": "{}/{}.root".format(outDir, d),  
                            "input-files": [f["output-file"] for f in tasks if d in f["name"] and systName in f["name"]],
                            "dependencies": [f["name"] for f in tasks if d in f["name"] and systName in f["name"]],
                            "dCache": "{}/merged/{}".format(config["dCache"].replace("[E]", config["era"]).replace("[P]", d), systName) if "dCache" in config else "", 
                        }

                        tasks.append(MergeSkim(task))

        return tasks
