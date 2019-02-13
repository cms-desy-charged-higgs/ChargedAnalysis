import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module

class puWeightProducer(Module):
    def __init__(self, era):
        self.isData = True
        self.era = era

        self.fileDir = "{}/src/ChargedHiggs/nanoAOD_processing/data/pileUp/".format(os.environ["CMSSW_BASE"])
        
        self.files = {
            2017: "data_pileUp2017.root",
        }


    def beginJob(self):
        pass
        
    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree
        self.out.branch("puWeight", "F")

        self.pileUpWeight = ROOT.TH1F("puWeight", "puWeight", 100, 0, 100)

        if hasattr(inputTree, "GenPart_pt"):
            ROOT.gROOT.SetBatch(ROOT.kTRUE)

            self.isData = False

            pileUpFile = ROOT.TFile(self.fileDir + self.files[self.era])
            pileUp = pileUpFile.Get("pileup")

            puMC = ROOT.TH1F("puMC", "puMC", 100, 0, 100) 
            inputTree.Draw("Pileup_nPU>>puMC")

            pileUp.Scale(1./pileUp.Integral(), "width")
            puMC.Scale(1./puMC.Integral(), "width")

            self.pileUpWeight.Divide(pileUp, puMC)

            pileUpFile.Close()
            outputFile.cd()

    def analyze(self, event):
        if not self.isData:
            pileUp = Object(event, "Pileup") 
            
            puWeight = self.pileUpWeight.GetBinContent(self.pileUpWeight.FindBin(pileUp.nPU))
            self.out.fillBranch("puWeight", puWeight)

        return True

cHiggspuWeightProducer = lambda era : puWeightProducer(era)

