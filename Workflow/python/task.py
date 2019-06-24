from abc import ABCMeta, abstractmethod
import yaml
import os

class Task(dict):
    __metaclass__ = ABCMeta

    def __init__(self, config = {}):
        ##Default values
        self["name"] = "Task"
        self["status"] = "VALID"
        self["dir"] = os.getcwd() + "/"
        self["dependencies"] = []

        ##Update with input config
        self.update(config)

        if "display-name" not in self:
            self["display-name"] = self["name"]

        self._allowParallel = True

    def __hash__(self):
        return id(self)

    def __call__(self):
        self.run()
    
    def getDependentFiles(self, depGraph):
        if self["tasklayer"] == 0:
            return 0

        ##Loop over all dependecies and save information
        for task in depGraph[self["tasklayer"]-1]:
            if task["name"] in self["dependencies"] and type(task["output"]) == str:
                if len(task["output"]) != 0:
                    self.setdefault("dependent_files", []).append(task["output"])

    def dump(self):
        ##Dump this task config in yaml file
        with open("{}/{}.yaml".format(self["dir"], self["name"]), "w") as configFile:
            yaml.dump(dict(self), configFile, default_flow_style=False, width=float("inf"), indent=4)

    def createDir(self):
        ##Create directory of this task
        if(not os.path.exists(self["dir"])):
            os.system("mkdir -p {}".format(self["dir"])) 
            print("Created task dir: {}".format(self["dir"]))

    @property
    def allowParallel(self):
        return self._allowParallel

    @allowParallel.setter
    def tasks(self, decision):
        self._allowParallel = decision

    ##Abstract functions which has to be overwritten
    @abstractmethod
    def status(self):
        pass

    @abstractmethod
    def run(self):
        pass

    @abstractmethod
    def output(self):
        pass
