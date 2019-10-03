from task import Task

import os

class Plot1D(Task):
    def __init__(self, config = {}):
        Task.__init__(self, config)

    def status(self):
        if not False in [os.path.isfile(output) for output in self["output"]]:
            self["status"] = "FINISHED"

    def run(self):
        ##Run the Plotter1D executable
        os.system("Plot1D {} '{}' {} '{}' '{}'".format(
                self["hist-dir"], 
                " ".join(self["x-parameter"]),  
                self["channel"],    
                " ".join(self["processes"]), 
                " ".join([self["dir"]]))
        )

    def output(self):
        self["output"] = ["{}/{}.pdf".format(self["dir"], x) for x in self["x-parameter"]]

    @staticmethod
    def configure(treeTasks, conf):
        chanToDir = {"mu4j": "Muon4J", "e4j": "Ele4J", "mu2j1f": "Muon2J1F", "e2j1f": "Ele2J1F", "mu2f": "Muon2F", "e2f": "Ele2F"}
        
        tasks = []

        for channel in conf.keys():
            plotConf = {"name": "Plot_{}".format(channel), 
                        "channel": channel, 
                        "hist-dir": os.environ["CHDIR"] + "/Hist/{}/{}".format(conf[channel]["dir"], chanToDir[channel]), 
                        "dir":  os.environ["CHDIR"] + "/CernWebpage/Plots/{}/{}".format(conf[channel]["dir"], chanToDir[channel]), 
                        "display-name": "Plots: {}".format(channel), 
                        "x-parameter": conf[channel]["x-parameter"], 
                        "dependencies": [t["name"] for t in treeTasks if t["channel"] == channel], 
                        "processes": [t["process"] for t in treeTasks if t["channel"] == channel]
            }

            tasks.append(Plot1D(plotConf))

        return tasks
