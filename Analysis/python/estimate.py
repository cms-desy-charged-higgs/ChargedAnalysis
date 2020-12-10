from task import Task

import os

class Estimate(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "estimate"

        self["arguments"] = [
            "--processes", *self["processes"],
            "--parameter", self["parameter"],
            "--out-dir", self["dir"]
        ]
        
        for process in self["processes"]:
            self["arguments"].append("--{}-bkg-files".format(process))
            self["arguments"].extend(self["{}-bkg-files".format(process)])
            
            self["arguments"].append("--{}-data-file".format(process))
            self["arguments"].append(self["{}-data-file".format(process)])


    def output(self):
        self["output"] = ["{}/{}.pdf".format(self["dir"], "kappa")]

    @staticmethod
    def configure(config, haddTasks, channel, era):
        outDir = os.environ["CHDIR"] + "/" + config["dir"].replace("[E]", era).replace("[C]", channel)
        data = config["data"][channel][0]
    
        task = {
                "name": "Estimate_{}_{}".format(channel, era),
                "processes": list(config["estimate-process"].keys()),
                "parameter": config["parameter"],
                "dependencies": [t["name"] for t in haddTasks],
                "dir": outDir,
        }
          
        for process in config["estimate-process"].keys():
            fileNames = ["{}/{}".format(t["dir"], t["out-name"]) for t in haddTasks if "{}-Region".format(process) in t["dir"]]
                  
            for p in reversed(config["estimate-process"]):
                for idx, f in enumerate(fileNames):
                    if p in f.split("/")[-1]:
                        fileNames.pop(idx)
                        fileNames.insert(0, f)
                        
                    elif data in f.split("/")[-1]:
                        dataFile = fileNames.pop(idx)
                        
            task["{}-bkg-files".format(process)] = fileNames
            task["{}-data-file".format(process)] = dataFile
          
        return [Estimate(task)]
