import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class electronSelector(Module):
    def __init__(self, era, ptCut, etaCut):

        self.isData = True
        self.era = era
        self.ptCut = ptCut
        self.etaCut = etaCut

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

        self.intervall = lambda (low, high), x: low <= x < high

        #https://twiki.cern.ch/twiki/pub/CMS/Egamma2017DataRecommendations/egammaEffi.txt_egammaPlots_runBCDEF_passingRECO.pdf

        self.recoPtBinning = {
            2017: [(20., 45.), (45., 75.), (75., 100.), (100., 500.)],     
        }

        self.recoEtaBinning = {
            2017: [(-2.500, -2.000), (-2.000, -1.566), (-1.566, -1.444), (-1.444, -1.000), (-1.000, -0.500), (-0.500, 0.000), (0.000, 0.500), (0.500, 1.000), (1.000, 1.444), (1.444, 1.566), (1.566, 2.000), (2.000, 2.500)],  
        }

        #https://soffi.web.cern.ch/soffi/EGM-ID/SF-17Nov2017-MCv2-IDv1-020618/Electrons/gammaEffi.txt_egammaPlots_runBCDEF_passingMVA94Xwp90iso.pdf

        self.mvaPtBinning = {
            2017: [(10., 20.), (20., 35.), (35., 50.), (50., 100.), (100., 200.), (200., 500.)],     
   
        }

        self.mvaEtaBinning = {
            2017: [(-2.500, -2.000), (-2.000, -1.566), (-1.444, -1.000), (-1.000, -0.500), (-0.500, 0.000), (0.000, 0.500), (0.500, 1.000), (1.000, 1.444), (1.566, 2.000), (2.000, 2.500)], 
        }

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        ## Load Lepton class
        ROOT.gROOT.ProcessLine(".L {}/src/ChargedHiggs/nanoAOD_processing/macros/lepton.cc".format(os.environ["CMSSW_BASE"]))

        ##Branch with vector of electrons
        self.eleVec = ROOT.std.vector(ROOT.Lepton)()
        self.eleBranch = self.out.tree().Branch("electron", self.eleVec)

        if "RunIIFall" in inputFile.GetName():
            self.isData = False
            
            ## 2D hist for SF for reco
            self.recoSFfile = ROOT.TFile(self.fileDir + self.recoSFfiles[self.era])
            self.recoSFhist = self.recoSFfile.Get("EGamma_SF2D")

            ## 2D hist for SF for MVA
            self.mvaMediumSFfile = ROOT.TFile(self.fileDir + self.mvaSFfiles[self.era]["medium"])
            self.mvaMediumSFhist = self.mvaMediumSFfile.Get("EGamma_SF2D")

            self.mvaTightSFfile = ROOT.TFile(self.fileDir + self.mvaSFfiles[self.era]["tight"])
            self.mvaTightSFhist = self.mvaTightSFfile.Get("EGamma_SF2D")
   
            ## Vector and branches for SF
            self.recoSFvec = ROOT.std.vector("float")()
            self.mvaMediumSFvec = ROOT.std.vector("float")()
            self.mvaTightSFvec = ROOT.std.vector("float")()

            self.recoSFbranch = self.out.tree().Branch("recoSF", self.recoSFvec)
            self.mvaMediumSFbranch = self.out.tree().Branch("mvaMediumSF", self.mvaMediumSFvec)
            self.mvaTightSFbranch = self.out.tree().Branch("mvaTightSF", self.mvaTightSFvec)

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        ##Clear vectors
        self.eleVec.clear()
        if not self.isData:
            self.recoSFvec.clear()
            self.mvaMediumSFvec.clear()    
            self.mvaTightSFvec.clear()

        ##Get electrons of Tree
        electrons = Collection(event, "Electron")

        ##Loop over jets and set all information
        for electron in electrons:
            if electron.pt > self.ptCut and abs(electron.eta) < self.etaCut: 
                validElectron = ROOT.Lepton()
                
                ele4Vec = ROOT.TLorentzVector()
                ele4Vec.SetPtEtaPhiM(electron.pt, electron.eta, electron.phi, electron.mass)

                validElectron.fourVec = ele4Vec
                validElectron.chage = electron.charge
                validElectron.isMedium = self.mvaID[self.era]["medium"](electron)
                validElectron.isTight = self.mvaID[self.era]["tight"](electron)

                self.eleVec.push_back(validElectron)

                ##Calculate SF for reco and MVA
                if not self.isData:
                    if electron.pt < 500. and not 1.444 < abs(electron.eta) < 1.566:

                        recoPtBin = [self.intervall(self.recoPtBinning[self.era][index], electron.pt) for index in range(len(self.recoPtBinning[self.era]))].index(True)

                        recoEtaBin = [self.intervall(self.recoEtaBinning[self.era][index], electron.eta) for index in range(len(self.recoEtaBinning[self.era]))].index(True)

                        mvaPtBin = [self.intervall(self.mvaPtBinning[self.era][index], electron.pt) for index in range(len(self.mvaPtBinning[self.era]))].index(True)

                        mvaEtaBin = [self.intervall(self.mvaEtaBinning[self.era][index], electron.eta) for index in range(len(self.mvaEtaBinning[self.era]))].index(True)

                        recoSF = self.recoSFhist.GetBinContent(recoEtaBin, recoPtBin)
                        mvaMediumSF = self.mvaMediumSFhist.GetBinContent(mvaEtaBin, mvaPtBin)
                        mvaTightSF = self.mvaTightSFhist.GetBinContent(mvaEtaBin, mvaPtBin)
                        
                        self.recoSFvec.push_back(recoSF if recoSF!=0 else 1.)
                        self.mvaMediumSFvec.push_back(mvaMediumSF if mvaMediumSF!=0 else 1.) 
                        self.mvaTightSFvec.push_back(mvaTightSF if mvaTightSF!=0 else 1.)

                    else:
                        self.recoSFvec.push_back(1.)
                        self.mvaMediumSFvec.push_back(1.)    
                        self.mvaTightSFvec.push_back(1.)
                        
        ##Fill branches
        self.eleBranch.Fill()
    
        if not self.isData:
            self.recoSFbranch.Fill()
            self.mvaMediumSFbranch.Fill()
            self.mvaTightSFbranch.Fill()

        return True
       
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cHiggsElectronSelector = lambda era, ptCut, etaCut: electronSelector(era = era, ptCut = ptCut, etaCut = etaCut) 
