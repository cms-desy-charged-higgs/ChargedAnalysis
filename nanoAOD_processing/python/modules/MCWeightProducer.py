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

        if "RunIIFall" in inputFile.GetName():

            self.out.branch("lumi", "F")
            self.out.branch("xsec", "F")
            self.out.branch("genWeight", "F")

            self.isData = False
            
            self.processname = inputFile.GetName().split("/")[7]
            self.processdic = yaml.load(file("{}/src/ChargedHiggs/nanoAOD_processing/data/xsec.yaml".format(os.environ["CMSSW_BASE"]), "r"))
            
    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        if not self.isData:
            self.out.fillBranch("lumi", self.lumi[self.era])
            self.out.fillBranch("xsec", self.processdic[self.processname]["xsec"])
            self.out.fillBranch("genWeight", event.genWeight)
            

        return True
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cHiggsMCWeightProducer = lambda era: MCWeightProducer(era = era) 
