from ChargedHiggs.Workflow.task import Task

import os

from ROOT import Plotter1D, string, vector

class Plot1D(Task):
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

    def status(self):
        if not False in [os.path.isfile(output) for output in self["output"]]:
            self["status"] = "FINISHED"

    def run(self):
        self.__toStd()

        plotter = Plotter1D(self._stdDir["hist-dir"], self._stdDir["x-parameter"], self._stdDir["channel"])
        plotter.ConfigureHists(self._stdDir["processes"])
        plotter.Draw(vector("string")(1, self._stdDir["dir"]))


    def output(self):
        self["output"] = ["{}/{}.pdf".format(self["dir"], x) for x in self["x-parameter"]]
