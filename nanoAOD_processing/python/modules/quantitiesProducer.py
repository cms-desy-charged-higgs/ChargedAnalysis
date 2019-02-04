import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module

class quantitiesProducer(Module):
    def __init__(self):
        self.isData = True

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out =  wrappedOutputTree 

        self.quantities = ROOT.Quantities()
        self.quantitiesBranch = self.out.tree().Branch("quantities", self.quantities)
        
        
    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        PV = Object(event, "PV")

        self.quantities.HT = event.SoftActivityJetHT
        self.quantities.PV = ROOT.TVector3(PV.x, PV.y, PV.z)
        self.quantitiesBranch.Fill()

        return True
        
cHiggsQuantitiesProducer = lambda : quantitiesProducer() 
