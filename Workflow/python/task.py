from abc import ABC, abstractmethod
import yaml
import os
import shutil

class Task(ABC, dict):
    def __init__(self, config = {}):
        ##Default values
        self["name"] = "Task_{}".format(id(self))
        self["status"] = "VALID"
        self["dir"] = os.getcwd() + "/"
        self["dependencies"] = []
        self["run-mode"] = "Local"

        ##Update with input config
        self.update(config)

        if "display-name" not in self:
            self["display-name"] = self["name"]

    def __hash__(self):
        return id(self)

    def __call__(self):
        self.run()
    
    def createCondor(self):
        ##Create directory with condor
        self["condor-dir"] = "{}/Condor/{}".format(self["dir"], self["name"])
        shutil.rmtree(self["condor-dir"], ignore_errors=True)
        os.makedirs(self["condor-dir"], exist_ok=True)

        ##Dic which will be written into submit file
        condorDic = {
            "universe": "vanilla",
            "executable": "{}/condor.sh".format(self["condor-dir"]),
            "transfer_executable": "True",
            "getenv": "True",
            "log": "{}/condor.log".format(self["condor-dir"]),
            "error": "{}/condor.err".format(self["condor-dir"]),
            "output":"{}/condor.out".format(self["condor-dir"]),
        }

        ##Write submit file
        with open("{}/condor.sub".format(self["condor-dir"]), "w") as condFile:
            for key, value in condorDic.items():
                condFile.write("{} = {}\n".format(key, value))

            condFile.write("queue 1")

        ##Write executable
        fileContent = [
                        "#!/bin/bash\n", 
                        "cd $CHDIR\n",
                        "echo 'You are in directory: $(pwd)'\n"
                        "source ChargedAnalysis/setenv.sh StandAlone\n",
                        "echo 'Exetubale is called: {}'\n".format(self["executable"]),
                        "{} {}".format(self["executable"], " ".join("'{}'".format(i) for i in self["arguments"]))
        ]

        with open("{}/condor.sh".format(self["condor-dir"]), "w") as condExe:
            for line in fileContent:
                condExe.write(line)

        os.system("chmod a+x {}/condor.sh".format(self["condor-dir"]))

    def getDependentFiles(self, depGraph):
        if self["tasklayer"] == 0:
            return 0

        ##Loop over all dependecies and save information
        for task in depGraph[self["tasklayer"]-1]:
            if task["name"] in self["dependencies"] and type(task["output"]) == str:
                if len(task["output"]) != 0:
                    self.setdefault("dependent-files", []).append(task["output"])

    def dump(self):
        ##Dump this task config in yaml file
        with open("{}/{}.yaml".format(self["dir"], self["name"]), "w") as configFile:
            yaml.dump(dict(self), configFile, default_flow_style=False, width=float("inf"), indent=4)

    def createDir(self):
        ##Create directory of this task
        if(not os.path.exists(self["dir"])):
            os.system("mkdir -p {}".format(self["dir"])) 
            print("Created task dir: {}".format(self["dir"]))

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
