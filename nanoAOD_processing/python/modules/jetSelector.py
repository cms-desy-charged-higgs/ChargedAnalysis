import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module

class jetSelector(Module):
    def __init__(self, era, ptCut, etaCut):
        self.isData = True
        self.era = era

        self.ptCut = ptCut
        self.etaCut = etaCut

        ##https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation94X

        self.bTag = {
            2017: {
                    "loose": 0.051, 
                    "medium": 0.3033, 
                    "tight": 0.7489,
            }
        }

        self.csv = {
                    2017: 'ChargedHiggs/nanoAOD_processing/data/btagSF/DeepFlavour_94XSF_V1_B_F.csv',
        }

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out =  wrappedOutputTree 

        ## Load Jet class
        ROOT.gROOT.ProcessLine(".L {}/src/ChargedHiggs/nanoAOD_processing/macros/jet.cc".format(os.environ["CMSSW_BASE"]))

        ##Branch with vector of jet
        self.jetVec = ROOT.std.vector(ROOT.Jet)()
        self.jetBranch = self.out.tree().Branch("jet", self.jetVec)

        ##bTag SF reader
        if "RunIIFall" in inputFile.GetName():
            self.isData = False

            ##Vector with SF
            self.SFvec = ROOT.std.vector("float")()
            self.SFbranch = self.out.tree().Branch("bJetSF", self.SFvec)
       
            ##https://twiki.cern.ch/twiki/bin/view/CMS/BTagCalibration

            ROOT.gSystem.Load('libCondFormatsBTauObjects') 
            ROOT.gSystem.Load('libCondToolsBTau')

            self.calib = ROOT.BTagCalibration('deepflavour', self.csv[self.era])

            self.v_sys = ROOT.std.vector("string")()
            self.v_sys.push_back('up')
            self.v_sys.push_back('down')

            # make a reader instance and load the sf data
            self.reader = ROOT.BTagCalibrationReader(0, "central", self.v_sys)    
            self.reader.load(self.calib, 0, "comb")        

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        ##Clear vectors
        self.jetVec.clear()
        if not self.isData:
            self.SFvec.clear()

        ##Get jets collection from Tree
        jets = Collection(event, "Jet")

        ##Loop over jets and set all information
        for jet in jets:
            if jet.pt > self.ptCut and abs(jet.eta) < self.etaCut:
                validJet = ROOT.Jet()
                
                jet4Vec = ROOT.TLorentzVector()
                jet4Vec.SetPtEtaPhiM(jet.pt, jet.eta, jet.phi, jet.mass)

                validJet.fourVec = jet4Vec
                validJet.isLooseB = self.bTag[self.era]["loose"] > jet.btagDeepFlavB
                validJet.isMediumB = self.bTag[self.era]["medium"] > jet.btagDeepFlavB
                validJet.isTightB = self.bTag[self.era]["tight"] > jet.btagDeepFlavB

                self.jetVec.push_back(validJet)

                ##Calculate bTag SF
                if not self.isData:
                    SF = self.reader.eval_auto_bounds('central', 0, abs(jet.eta), jet.pt)
                    
                    if SF != 0: 
                        self.SFvec.push_back(SF)
                    else:
                        self.SFvec.push_back(1.)

        #Fill branches
        self.jetBranch.Fill()
        
        if not self.isData:
            self.SFbranch.Fill()        

        return True
        
cHiggsJetSelector = lambda era, ptCut, etaCut: jetSelector(era = era, ptCut = ptCut, etaCut = etaCut) 
