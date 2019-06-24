from ChargedHiggs.Workflow.task import Task

import os

from ROOT import PlotterLimit, string, vector

class LimitPlot(Task):
    def __init__(self, config = {}):
        Task.__init__(self, config)

    def __toStd(self):
        self._stdDir = {}

        ##Translate string and list to std::string and std::vector
        for key, value in self.iteritems():
            if type(value) == str:
                self._stdDir[key] = string(value)

            elif type(value) == list:
                if type(value[0]) == str:
                    self._stdDir[key] = vector("string")()

                elif type(value[0]) == float:   
                    self._stdDir[key] = vector("float")()

                elif type(value[0]) == int:   
                    self._stdDir[key] = vector("int")()
                
                for v in value:
                   self._stdDir[key].push_back(v) 

            else:
                pass

    def status(self):
        if os.path.isfile(self["output"]):
            self["status"] = "FINISHED"

    def run(self):
        self.__toStd()

        plotter = PlotterLimit(self._stdDir["dir"], self._stdDir["masses"])
        plotter.ConfigureHists(vector("string")())
        plotter.Draw(vector("string")(1, self._stdDir["dir"]))

    def output(self):
        self["output"] = self["dir"] + "/limit.pdf"
