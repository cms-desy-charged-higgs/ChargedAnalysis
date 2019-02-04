import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import yaml
import os

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class MCWeightProducer(Module):
    def __init__(self, era):
        self.era = era

        self.lumi = {
                    2016: 35.87,
                    2017: 41.37,
        }

        self.isData = True

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        if "mc" in inputFile.GetName() or "user" in inputFile.GetName():
            self.isData = False

            self.out.branch("lumi", "F")
            self.out.branch("xsec", "F")
            self.out.branch("genWeight", "F")

            self.processdic = yaml.load(file("{}/src/ChargedHiggs/nanoAOD_processing/data/xsec.yaml".format(os.environ["CMSSW_BASE"]), "r"))

            self.xsec = 1.

            for key in self.processdic.keys():
                if key in inputFile.GetName():
                    self.xsec = self.processdic[key]["xsec"]
            
    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        if not self.isData:
            self.out.fillBranch("lumi", self.lumi[self.era])
            self.out.fillBranch("xsec", self.xsec)
            self.out.fillBranch("genWeight", event.genWeight)
            

        return True
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cHiggsMCWeightProducer = lambda era: MCWeightProducer(era = era) 
