from ChargedHiggs.Workflow.task import Task

import os

from ROOT import Limit, vector, string

class LimitTask(Task):
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

        limit = Limit(self._stdDir["mass"], self._stdDir["channel"], self._stdDir["background"], self._stdDir["dir"])
        limit.WriteDatacard(self._stdDir["hist-dir"], self._stdDir["x-parameter"])
        limit.CalcLimit()

    def output(self):
        self["output"] = self["dir"] + "/limit.root"

    def status(self):
        if os.path.isfile(self["output"]):
            self["status"] = "FINISHED"
