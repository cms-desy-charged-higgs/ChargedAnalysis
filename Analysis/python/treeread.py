from ChargedHiggs.Workflow.task import Task

import os

from ROOT import TreeReader, string, vector

class TreeRead(Task):
    def __init__(self, config = {}):
        Task.__init__(self, config)

        if not "y-parameter" in self:
            self["y-parameter"] = []

        if not "event-fraction" in self:
            self["event-fraction"] = 1.0

        if not "write-tree" in self:
            self["write-tree"] = False

        if not "write-tree" in self:
            self["write-csv"] = False

        self._allowParallel = False

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
        if os.path.isfile(self["output"]):
            self["status"] = "FINISHED"
        
    def run(self):
        self.__toStd()

        ##Run the reader
        reader = TreeReader(self._stdDir["process"], self._stdDir["x-parameter"], self._stdDir["y-parameter"], self._stdDir["cuts"], self._stdDir["output"],  self._stdDir["channel"], self["write-tree"], self["write-csv"])
        reader.Run(self._stdDir["filenames"], self["event-fraction"])
        reader.Merge()

    def output(self):
        self["output"] = "{}/{}.root".format(self["dir"], self["process"])
