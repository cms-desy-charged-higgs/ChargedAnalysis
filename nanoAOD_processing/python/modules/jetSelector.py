import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os
import numpy as np

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module

class jetSelector(Module):
    def __init__(self, era, ptCut, etaCut, minNJet):
        self.isData = True
        self.era = era

        self.ptCut = ptCut
        self.etaCut = etaCut
        self.minNJet = minNJet

        ##https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation94X
        self.csv = {
                    2017: "{}/src/ChargedHiggs/nanoAOD_processing/data/btagSF/DeepFlavour_94XSF_V1_B_F.csv".format(os.environ["CMSSW_BASE"]),
        }

        self.bTag = {
            2017: {
                    "loose": 0.051, 
                    "medium": 0.3033, 
                    "tight": 0.7489,
            }
        }

        self.jme = {
                    2017: [
                    "{}/src/ChargedHiggs/nanoAOD_processing/data/JME/Fall17_V3_MC_PtResolution_AK4PFchs.txt".format(os.environ["CMSSW_BASE"]),
                    "{}/src/ChargedHiggs/nanoAOD_processing/data/JME/Fall17_V3_MC_SF_AK4PFchs.txt".format(os.environ["CMSSW_BASE"])
                    ]
        }

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out =  wrappedOutputTree 

        ## Function for reading jet resolution SF        
        ROOT.gROOT.ProcessLine(".L {}/src/ChargedHiggs/nanoAOD_processing/macros/jetSmearer.cc".format(os.environ["CMSSW_BASE"]))

        ##Branch with vector of jet
        self.jetVec = ROOT.std.vector(ROOT.Jet)()
        self.jetBranch = self.out.tree().Branch("jet", self.jetVec)

        self.MET = ROOT.TLorentzVector()
        self.METBranch = self.out.tree().Branch("MET", self.MET)

        ##bTag SF reader
        if hasattr(inputTree, "GenPart_pt"):
            self.isData = False

            self.jetSmearer = ROOT.JetSmearer(ROOT.std.string(self.jme[self.era][0]), ROOT.std.string(self.jme[self.era][1]))
       
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

    def smearer(self, jet, genJets, rho):
        ##https://twiki.cern.ch/twiki/bin/viewauth/CMS/JetResolution#Smearing_procedures
        self.jetSmearer.SetJet(jet.pt, jet.eta, rho)

        reso = self.jetSmearer.GetReso()
        resoSF = self.jetSmearer.GetResoSF()    

        smearFac = 1.
        matchedGenJet = None

        for genJet in genJets:
            dR = np.sqrt((jet.phi-genJet.phi)**2 + (jet.eta-genJet.eta)**2)
             
            if dR < 0.4/2 and abs(jet.pt - genJet.pt) < 3.*reso*jet.pt:
                matchedGenJet = genJet 

        if matchedGenJet:
            smearFac = 1+(resoSF-1)*(jet.pt - genJet.pt)/jet.pt

        elif resoSF > 1.:
            sigma = reso*np.sqrt(resoSF**2 - 1.)
            smearFac = np.random.normal(1., sigma, 1)[0]

        else:
            pass

        if smearFac*jet.pt < 1.e-2:
             smearFac = 1.e-2

        return smearFac
            

    def analyze(self, event):
        ##Clear vectors
        self.jetVec.clear()

        ##Get jets collection from Tree
        jets = Collection(event, "Jet")
        MET = Object(event, "MET")

        if not self.isData:
            genJets = Collection(event, "GenJet")

        metPx = MET.pt*np.cos(MET.phi)
        metPy = MET.pt*np.sin(MET.phi)

        ##Loop over jets and set all information
        for jet in jets:
            if not self.isData:
                smearValue = self.smearer(jet, genJets, event.fixedGridRhoFastjetAll)
                jetPt = jet.pt*smearValue

                metPx = metPx - jet.pt*np.cos(jet.phi) + jetPt*np.cos(jet.phi)
                metPy = metPy - jet.pt*np.sin(jet.phi) + jetPt*np.sin(jet.phi)

            else:
                jetPt = jet.pt

            if jetPt > self.ptCut and abs(jet.eta) < self.etaCut and jet.electronIdx1 == -1 and jet.muonIdx1 == -1:
                validJet = ROOT.Jet()
                
                jet4Vec = ROOT.TLorentzVector()
                jet4Vec.SetPtEtaPhiM(jetPt, jet.eta, jet.phi, jet.mass)

                self.MET.SetPtEtaPhiM(MET.pt, 0, MET.phi, 0)
                self.METBranch.Fill()

                validJet.fourVec = jet4Vec
                validJet.isLooseB = self.bTag[self.era]["loose"] < jet.btagDeepFlavB
                validJet.isMediumB = self.bTag[self.era]["medium"] < jet.btagDeepFlavB
                validJet.isTightB = self.bTag[self.era]["tight"] < jet.btagDeepFlavB

                ##Calculate bTag SF
                if not self.isData:
                    SF = self.reader.eval_auto_bounds('central', 0, abs(jet.eta), jetPt)
                    validJet.bTagSF = SF

                self.jetVec.push_back(validJet)

        if(self.jetVec.size() < self.minNJet):
            return False

        #Fill branches
        self.MET.SetPxPyPzE(metPx, metPy, 0, 0)

        self.METBranch.Fill()
        self.jetBranch.Fill()

        return True
        
cHiggsJetSelector = lambda era, ptCut, etaCut, minNJet: jetSelector(era = era, ptCut = ptCut, etaCut = etaCut, minNJet=minNJet) 
