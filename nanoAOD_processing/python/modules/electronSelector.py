import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os
import numpy as np

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class electronSelector(Module):
    def __init__(self, era, ptCut, etaCut, minNEle, triggerBit):

        self.isData = True
        self.era = era
        self.ptCut = ptCut
        self.etaCut = etaCut
        self.minNEle = minNEle
        self.triggerBit = triggerBit

        ## https://twiki.cern.ch/twiki/bin/view/CMS/MultivariateElectronIdentificationRun2
        self.mvaID = {
                    2017: { 
                            "medium": lambda x: x.mvaFall17Iso_WP80, 
                            "tight": lambda x: x.mvaFall17Iso_WP90,
                    }
        }

        self.fileDir = "{}/src/ChargedHiggs/nanoAOD_processing/data/eleSF/".format(os.environ["CMSSW_BASE"])
        
        self.recoSFfiles = {
            2017: "egammaEffi.txt_EGM2D_runBCDEF_passingRECO.root",
        }

        
        self.mvaSFfiles = {
            2017: {  
                    "medium": "gammaEffi.txt_EGM2D_runBCDEF_passingMVA94Xwp80iso.root", 
                    "tight": "gammaEffi.txt_EGM2D_runBCDEF_passingMVA94Xwp90iso.root", 
            }
        }


    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        ##Branch with vector of electrons
        self.eleVec = ROOT.std.vector(ROOT.Electron)()
        self.eleBranch = self.out.tree().Branch("electron", self.eleVec)

        if "mc" in inputFile.GetName() or "user" in inputFile.GetName():
            self.isData = False
            
            ## 2D hist for SF for reco
            self.recoSFfile = ROOT.TFile(self.fileDir + self.recoSFfiles[self.era])
            self.recoSFhist = self.recoSFfile.Get("EGamma_SF2D")

            ## 2D hist for SF for MVA
            self.mvaMediumSFfile = ROOT.TFile(self.fileDir + self.mvaSFfiles[self.era]["medium"])
            self.mvaMediumSFhist = self.mvaMediumSFfile.Get("EGamma_SF2D")

            self.mvaTightSFfile = ROOT.TFile(self.fileDir + self.mvaSFfiles[self.era]["tight"])
            self.mvaTightSFhist = self.mvaTightSFfile.Get("EGamma_SF2D")
   

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def triggerMatching(self, event, eleVec, filterBit):
        trigObject = Collection(event, "TrigObj")

        if filterBit == 2:
            for obj in trigObject:
                if obj.id == 11:
                    matched = [5e-2 > np.sqrt((obj.eta - electron.fourVec.Eta())**2 + (obj.phi - electron.fourVec.Phi())**2) for electron in eleVec]

        return True in matched


    def analyze(self, event):
        ##Clear vectors
        self.eleVec.clear()

        ##Get electrons of Tree
        electrons = Collection(event, "Electron")

        ##Loop over jets and set all information
        for electron in electrons:
            if electron.pt > self.ptCut and abs(electron.eta) < self.etaCut and electron.convVeto: 
                validElectron = ROOT.Electron()

                #print electron.jetIdx
                
                ele4Vec = ROOT.TLorentzVector()
                ele4Vec.SetPtEtaPhiM(electron.pt, electron.eta, electron.phi, electron.mass)

                validElectron.fourVec = ele4Vec
                validElectron.charge = electron.charge
                validElectron.isolation = electron.pfRelIso03_all
                validElectron.isMedium = self.mvaID[self.era]["medium"](electron)
                validElectron.isTight = self.mvaID[self.era]["tight"](electron)

                ##Calculate SF for reco and MVA
                if not self.isData:
                    recoSF = self.recoSFhist.GetBinContent(self.recoSFhist.FindBin(electron.eta, electron.pt))
                    mvaMediumSF = self.mvaMediumSFhist.GetBinContent(self.mvaMediumSFhist.FindBin(electron.eta, electron.pt))
                    mvaTightSF = self.mvaTightSFhist.GetBinContent(self.mvaTightSFhist.FindBin(electron.eta, electron.pt))
                        
                    validElectron.recoSF = recoSF
                    validElectron.mediumMvaSF = mvaMediumSF
                    validElectron.tightMvaSF = mvaTightSF

                self.eleVec.push_back(validElectron)

        if(self.eleVec.size() < self.minNEle):
            return False

        ##Trigger matching
        if self.triggerBit!=None:
            if not self.triggerMatching(event, self.eleVec, self.triggerBit):
                return False
                        
        ##Fill branches
        self.eleBranch.Fill()

        return True
       
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cHiggsElectronSelector = lambda era, ptCut, etaCut, minNEle, triggerBit=None: electronSelector(era = era, ptCut = ptCut, etaCut = etaCut, minNEle=minNEle, triggerBit=triggerBit) 
