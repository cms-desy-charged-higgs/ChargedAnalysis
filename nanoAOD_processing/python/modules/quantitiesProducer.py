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
        
        self.MET = ROOT.TLorentzVector()
        self.METBranch = self.out.tree().Branch("MET", self.MET)
        
    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        ##Clear vectors
        MET = Object(event, "MET")

        self.MET.SetPtEtaPhiM(MET.pt, 0, MET.phi, 0)
        self.METBranch.Fill()
        
        return True
        
cHiggsQuantitiesProducer = lambda : quantitiesProducer() 
