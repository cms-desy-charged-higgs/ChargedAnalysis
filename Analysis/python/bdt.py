from ChargedHiggs.Workflow.task import Task

import os

from ROOT import BDT, vector, string

class BDTTask(Task):
    def __init__(self, config = {}):
        Task.__init__(self, config)  

    def __toStd(self):
        self._stdDir = {}

        ##Translate string and list to std::string and std::vector
        for key, value in self.iteritems():
            if type(value) == str:
                self._stdDir[key] = string(value)

            elif type(value) == list:
                self._stdDir[key] = vector("string")()
                
                for v in value:
                   self._stdDir[key].push_back(v) 

            else:
                pass

    def run(self):
        self.__toStd()

        bdt = BDT(1560, 1.1005293101958564, 0.49858931820896, 34, 5, 11, "CrossEntropy")
        bdt.Train(self._stdDir["x-parameter"], self._stdDir["tree-dir"], self._stdDir["dir"], self._stdDir["signal"], self._stdDir["background"], self._stdDir["event-type"]) 

    def output(self):
        self["output"] = self["dir"] + "/BDT.root"

    def status(self):
        if os.path.isfile(self["output"]):
            self["status"] = "FINISHED"
