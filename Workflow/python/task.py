import os
import yaml
import time
import subprocess

class Task(dict, object):
    def __init__(self, config = {}, argPrefix = "", beforeExe = [], afterExe = []):
        ##Set default values
        self["name"] = "Task_{}".format(id(self))
        self["dir"] = "Task_{}".format(id(self))
        self["dependencies"] = []
        self["status"] = "None"
        self["run-mode"] = "Local"

        ##Update config
        self.update(config)
        self.retries = 0

        ##Argument from constructor
        if not "argPrefix" in self:
            self["argPrefix"] = argPrefix

        if not "beforeExe" in self:
            self["beforeExe"] = beforeExe

        if not "afterExe" in self:
            self["afterExe"] = afterExe

        ##Check if all necessary things are provided
        for arg in ["executable", "arguments"]:
            if arg not in self:
                raise RuntimeError("Task is missing crucial information: {}".format(arg))

    def __hash__(self):
        return hash(self["name"])

    def prepare(self):
        ##Create directory of this task
        os.makedirs(self["dir"], exist_ok = True)

        ##Remove possible old out/err files
        for f in ["out", "log", "err"]:
            with open("{}/{}.txt".format(self["dir"], f), "w") as f: 
                f.write("")

        ##Create nice formatted shell script with exe and parser arguments
        with open("{}/run.sh".format(self["dir"]), "w") as exe:
            exe.write("#!/bin/bash\n\n")

            for idx, line in enumerate(self["beforeExe"]):
                exe.write(line)
                exe.write("\n")

                if len(self["beforeExe"]) == idx + 1:
                    exe.write("\n")

            exe.write("{} \\".format(self["executable"]))
            exe.write("\n")
    
            for arg, values in self["arguments"].items():
                exe.write("{}{}{} \\".format(2*" ", self["argPrefix"], arg))
                exe.write("\n")


                if type(values) == list:
                    for idx2, v in enumerate(values):
                        exe.write("{}{} {}".format(6*" ", v, "\\"))
                        exe.write("\n")

                else:
                    exe.write("{}{} {}".format(6*" ", values, "\\"))
                    exe.write("\n")

            for idx, line in enumerate(self["afterExe"]):
                if idx == 0:
                    exe.write(" ")
                    exe.write("\n")

                exe.write(line)
                exe.write("\n")

        subprocess.run("chmod a+x {}/run.sh".format(self["dir"]), shell = True)

        ##Dump self config in yaml format
        with open("{}/task.yaml".format(self["dir"]), "w") as task:
            yaml.dump(dict(self), task, default_flow_style=False, indent=4)

    def run(self):
        ##Set niceness super high
        os.nice(19)

        ##Call subproces and run executable
        with open("{}/out.txt".format(self["dir"]), "w") as out, open("{}/err.txt".format(self["dir"]), "w") as err, open("{}/log.txt".format(self["dir"]), "w") as log:
            log.write("[{}] Job started\n\n".format(time.asctime()))
            result = subprocess.run("source {}/run.sh".format(self["dir"]), shell = True, stdout = out, stderr = err)
            log.write("[{}] Job finished with returncode {}".format(time.asctime(), result.returncode))
                               
        return result.returncode
