from task import Task

import os
import yaml

class TreeRead(Task):
    def __init__(self, config = {}):
        Task.__init__(self, config)

        if not "y-parameter" in self:
            self["y-parameter"] = []

        if not "event-fraction" in self:
            self["event-fraction"] = 1.0

        if not "write-tree" in self:
            self["write-tree"] = False

        if not "write-csv" in self:
            self["write-csv"] = False

        self._allowParallel = False

    def status(self):
        if os.path.isfile(self["output"]):
            self["status"] = "FINISHED"
        
    def run(self):
        ##Run the TreeReader executable
        os.system("TreeRead {} '{}' '{}' '{}' {} {} {} {} '{}' {}".format(
                self["process"], 
                " ".join(self["x-parameter"]), 
                " ".join(self["y-parameter"]), 
                " ".join(self["cuts"]), 
                self["output"],  
                self["channel"],    
                self["write-tree"], 
                self["write-csv"], 
                " ".join(self["filenames"]), 
                self["event-fraction"])
        )

    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["process"])

    @staticmethod
    def configure(conf):
        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1f": "Muon2J1F", "e2j1f": "Ele2J1F", "mu2f": "Muon2F", "e2f": "Ele2F"}

        ##Dic with process:filenames 
        processDic = yaml.load(open("{}/ChargedAnalysis/Analysis/data/process.yaml".format(os.environ["CHDIR"]), "r"), Loader=yaml.Loader)

        skimDir = os.environ["CHDIR"] + "/Skim"
        tasks = []

        for channel in conf.keys():
            for process in conf[channel]["processes"]:
                ##List of filenames for each process
                filenames = ["{skim}/{file}/merged/{file}.root".format(skim=skimDir, file = processFile) for processFile in processDic[process]]  

                ##Configuration for treeread Task
                config = {"name": "Hist_{}_{}".format(channel, process), 
                          "display-name": "Hist: {} ({})".format(process, channel),
                          "channel": channel, 
                          "cuts": conf[channel]["cuts"], 
                          "dir":  os.environ["CHDIR"] + "/Hist/{}/{}".format(conf[channel]["dir"], chanToDir[channel]), 
                          "process": process, 
                          "x-parameter": conf[channel]["x-parameter"], 
                          "filenames": filenames,
                }

                tasks.append(TreeRead(config))

        return tasks
