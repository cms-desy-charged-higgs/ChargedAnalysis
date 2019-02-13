import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os
import numpy as np

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module

class topPtWeightProducer(Module):
    def __init__(self):
        self.isData = True
        self.nttPairs = 0

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out =  wrappedOutputTree 

        if hasattr(inputTree, "GenPart_pt"):
            self.isData = False

            self.out.branch("topPtWeight", "F")


    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        if not self.isData:

            genParticles = Collection(event, "GenPart")

            ttPair = 0
            topPt = []

            topPtWeight = 1.

            for genParticle in genParticles:
                if genParticle.pdgId == 6:
                    topPt.append(genParticle.pt)

            if len(topPt) == 2:
                topPtWeight = np.sqrt(np.exp(0.0615 - 0.0005*topPt[0])*np.exp(0.0615 - 0.0005*topPt[1]))

            self.out.fillBranch("topPtWeight", topPtWeight)
           
        return True
        
cHiggstopPtWeightProducer = lambda : topPtWeightProducer() 
