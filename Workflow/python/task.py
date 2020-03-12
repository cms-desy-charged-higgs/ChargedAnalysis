from abc import ABC, abstractmethod
import os
import subprocess

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

        ##List with dependent tasks
        self.dependencies = []

        ##Depth in the depedency graph
        self.depth = 0

        ##Check if prepare function is already called
        self.isPrepared = False

        if "display-name" not in self:
            self["display-name"] = self["name"]

    def __hash__(self):
        return id(self)

    def __call__(self):
        ##Remove possible old out/err files
        for f in ["out", "log", "err"]:
            subprocess.run(["command", "rm", "-rf", "{}/{}.txt".format(self["dir"], f)])

        ##Write shell exucutable with command
        fileContent = [
                        "#!/bin/bash\n", 
                        "source $CHDIR/ChargedAnalysis/setenv.sh Analysis\n",
                        " ".join([self["executable"], *[str(s) for s in self["arguments"]]])
        ]

        with open("{}/run.sh".format(self["dir"]), "w") as exe:
            for line in fileContent:
                exe.write(line)

        subprocess.run(["chmod", "a+x", "{}/run.sh".format(self["dir"])], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        ##Run locally if wished
        if self["run-mode"] == "Local":
            return self.runLocal()

    def runLocal(self):
        ##Set niceness super high
        os.nice(19)

        result = subprocess.run([self["executable"], *[str(s) for s in self["arguments"]]], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        ##Write out out/err
        with open("{}/out.txt".format(self["dir"]), "w") as out:
            for line in result.stdout.decode('utf-8'):
                out.write(line)

        with open("{}/err.txt".format(self["dir"]), "w") as err:
            for line in result.stderr.decode('utf-8'):
                err.write(line)

        return result.returncode

    def getDependentFiles(self):
        ##Loop over all dependecies and save information
        for task in self.dependencies:
            if type(task["output"]) == str:
                self.setdefault("dependent-files", []).append(task["output"])

            else:
                self.setdefault("dependent-files", []).extend(task["output"])

    def createDir(self):
        ##Create directory of this task
        os.makedirs(self["dir"], exist_ok=True)

    def checkOutput(self):
        self.output()

        ##Check if output already exists
        if type(self["output"]) == str:
            return os.path.exists(self["output"])

        else:
            for output in self["output"]:
                if not os.path.exists(output):
                    return False

            return True    

    def prepare(self):
        self.getDependentFiles()
        self.createDir()
        self.output()
        self.run()

        self.isPrepared=True
                            
    ##Abstract functions which has to be overwritten
    @abstractmethod
    def run(self):
        pass

    @abstractmethod
    def output(self):
        pass
