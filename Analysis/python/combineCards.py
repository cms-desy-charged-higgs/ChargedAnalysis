from task import Task

import os

class CombineCards(Task):       
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "combineCards.py"

        self["arguments"] = ["{}={}".format(l, card) for l,card in zip(self["labels"], self["input"]) if "txt" in card] + [">", self["dir"] + "/datacard.txt"]

    def output(self):
        self["output"] = self["dir"] + "/datacard.txt"
        

    @staticmethod
    def configure(config, cardTasks):
        tasks = []

        for era in config["era"]:
            for mHC in config["charged-masses"]:
                for mh in config["neutral-masses"]:
                    cardDir = "{}/{}/HPlus{}_h{}".format(os.environ["CHDIR"], config["dir"].replace("[E]", era).replace("[C]", "Combined"), mHC, mh)

                    dependencies = [t for t in cardTasks if t["era"] == era and str(mHC) in t["name"] and str(mh) in t["name"]]

                    conf = {
                        "name": "CombineCard_{}_{}_{}".format(era, mHC, mh), 
                        "dir": cardDir,
                        "display-name": "Combine Datacards {} ({}/{})".format(era, mHC, mh), 
                        "dependencies": [t["name"] for t in dependencies],
                        "input": [t["dir"] + "/datacard.txt" for t in dependencies],
                        "labels": config["channels"],
                        "era": era
                    }

                    tasks.append(CombineCards(conf))


        for mHC in config["charged-masses"]:
            for mh in config["neutral-masses"]:
                cardDir = "{}/{}/HPlus{}_h{}".format(os.environ["CHDIR"], config["dir"].replace("[E]", "RunII").replace("[C]", "Combined"), mHC, mh)

                dependencies = [t for t in cardTasks if str(mHC) in t["name"] and str(mh) in t["name"]]

                conf = {
                        "name": "CombineCard_{}_{}".format(mHC, mh), 
                        "dir": cardDir,
                        "display-name": "Combine Datacards ({}/{})".format(mHC, mh), 
                        "dependencies": [t["name"] for t in dependencies],
                        "input": [t["dir"] + "/datacard.txt" for t in dependencies],
                        "labels": ["{}_{}".format(t["channel"], t["era"]) for t in dependencies],
                }

                tasks.append(CombineCards(conf))
                
        return tasks
