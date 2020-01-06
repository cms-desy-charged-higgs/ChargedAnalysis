from task import Task

from ROOT import TFile, TTree

import os
import yaml
import numpy as np

class TreeRead(Task):
    def __init__(self, config = {}):
        super().__init__(config)

        if not "y-parameter" in self:
            self["y-parameter"] = []

        if not "save-mode" in self:
            self["save-mode"] = "Hist"

    def run(self):
        self["executable"] = "TreeRead"

        self["arguments"] = [
                self["process"], 
                "{}".format(" ".join(self["x-parameter"])), 
                "{}".format(" ".join(self["y-parameter"])), 
                "{}".format(" ".join(self["cuts"])), 
                self["output"],  
                self["channel"],    
                self["save-mode"],
                self["filename"], 
                "{}".format(' '.join(self["interval"]))
        ]

        return super()._run()
        
    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["name"])

    @staticmethod
    def configure(conf, channel):
        nEvents = 5*int(1e5) if not "number-events" in conf[channel] else conf[channel]["number-events"]

        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1f": "Muon2J1F", "e2j1f": "Ele2J1F", "mu2f": "Muon2F", "e2f": "Ele2F"}

        ##Dic with process:filenames 
        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        skimDir = os.environ["CHDIR"] + "/Skim"
        tasks = []

        for process in conf[channel]["processes"]:
            nJobs = 0

            ##List of filenames for each process
            filenames = ["{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = processFile) for processFile in processDic[process]]   

            for filename in filenames:
                rootFile = TFile.Open(filename)
                rootTree = rootFile.Get(channel)
                entries = rootTree.GetEntries()
                rootFile.Close()

                ##Complicated way of calculating intervals
                intervals = [["0", "0"]]

                if entries != 0:
                    nRange = np.arange(0, entries)
                    intervals = [[str(i[0]), str(i[-1])] for i in np.array_split(nRange, round(entries/nEvents+0.5))]

                for interval in intervals:
                    ##Configuration for treeread Task
                    config = {"name": "Hist_{}_{}_{}".format(channel, process, nJobs), 
                              "display-name": "Hist: {} ({})".format(process, channel),
                              "channel": channel, 
                              "cuts": conf[channel]["cuts"], 
                              "dir":  os.environ["CHDIR"] + "/Tmp/Hist/{}/{}".format(conf[channel]["dir"], chanToDir[channel]), 
                              "process": process, 
                              "x-parameter": conf[channel]["x-parameter"],
                              "filename": filename,
                              "interval": interval,  
                    }

                    if "run-mode" in conf[channel]:
                        config["run-mode"] = conf[channel]["run-mode"]

                    if "y-parameter" in conf[channel]:
                        config["y-parameter"] = conf[channel]["y-parameter"]

                    tasks.append(TreeRead(config))
                    nJobs+=1

        return tasks
