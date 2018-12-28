import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import yaml
import os

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class metFilter(Module):
    def __init__(self, era):
        self.era = era

        ##https://twiki.cern.ch/twiki/bin/viewauth/CMS/MissingETOptionalFiltersRun2

        self.filters = {
                2017: [
                    lambda event: event.goodVertices,
                    lambda event: event.globalSuperTightHalo2016Filter,
                    lambda event: event.HBHENoiseFilter,
                    lambda event: event.HBHENoiseIsoFilter,
                    lambda event: event.EcalDeadCellTriggerPrimitiveFilter,
                    lambda event: event.BadPFMuonFilter,
                    lambda event: event.BadChargedCandidateFilter,
                    lambda event: event.eeBadScFilter,
                ]
        }

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass
            
    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        flags = Object(event, "Flag")

        filters = [passedFilter(flags) for passedFilter in self.filters[self.era]]

        if False in filters:
            return False
            
        return True
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cMetFilter = lambda era: metFilter(era = 2017) 
