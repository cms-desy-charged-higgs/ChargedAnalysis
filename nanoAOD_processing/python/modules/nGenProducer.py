import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import yaml
import os

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class nGenProducer(Module):
    def __init__(self):
        self.isData = True

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree
        self.nGen = ROOT.TH1F("nGen", "nGen", 100, -1e7, 1e7)

        if hasattr(inputTree, "GenPart_pt"):
            self.nGenWeighted = ROOT.TH1F("nGenWeighted", "nGen", 100, -1e7, 1e7)
            self.isData = False
            
    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.nGen.Write()

        if not self.isData:
            self.nGenWeighted.Write()

    def analyze(self, event):
        self.nGen.Fill(1)

        if not self.isData:
            self.nGenWeighted.Fill(event.genWeight)

        return True
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cHiggsnGenProducer = lambda : nGenProducer()
