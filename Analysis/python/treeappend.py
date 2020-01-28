from task import Task

from ROOT import TFile, TTree

import os
import yaml
import numpy as np

class TreeAppend(Task):
    def __init__(self, config = {}):
        super().__init__(config)

    def run(self):
        self["executable"] = "TreeAppend"

        self["arguments"] = [
                "--old-file", self["input-file"], 
                "--old-tree", self["channel"],
                "--new-tree", self["output"],
                "--branch-names", *self["branch-names"],
                "--entry-start", self["entry-start"],  
                "--entry-end", self["entry-end"],
        ]

    def output(self):
        self["output"] = "{}/{}_{}.root".format(self["dir"], self["input-file"].split("/")[-1][:-5], self["entry-start"])
   
    @staticmethod
    def configure(conf, channel):
        nEvents = 5*int(1e5) if not "number-events" in conf[channel] else conf[channel]["number-events"]

        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1fj": "Muon2J1FJ", "e2j1fj": "Ele2J1FJ", "mu2fj": "Muon2FJ", "e2fj": "Ele2FJ"}

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
                    config = {"name": "Append_{}_{}_{}".format(channel, process, nJobs), 
                              "display-name": "Append: {} ({})".format(process, channel),
                              "channel": channel, 
                              "dir":  os.environ["CHDIR"] + "/Tmp/Append/{}".format(chanToDir[channel]), 
                              "input-file": filename, 
                              "entry-start": interval[0],  
                              "entry-end": interval[1],  
                              "branch-names": conf[channel]["branch-names"]
                    }

                    if "run-mode" in conf[channel]:
                        config["run-mode"] = conf[channel]["run-mode"]

                    tasks.append(TreeAppend(config))
                    nJobs+=1

        return tasks       
