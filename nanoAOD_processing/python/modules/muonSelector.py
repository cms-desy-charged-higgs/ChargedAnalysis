import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os
import numpy as np

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class muonSelector(Module):
    def __init__(self, era, ptCut, etaCut, minNMuon, triggerBit):

        self.isData = True
        self.era = era
        self.ptCut = ptCut
        self.etaCut = etaCut
        self.minNMuon = minNMuon
        self.triggerBit = triggerBit

        self.fileDir = "{}/src/ChargedHiggs/nanoAOD_processing/data/muonSF/".format(os.environ["CMSSW_BASE"])
        
        self.isoSFfiles = {
            2017: "RunBCDEF_SF_ISO.root",
        }

        self.triggerSFfiles = {
            2017: "EfficienciesAndSF_RunBtoF_Nov17Nov2017.root",
        }

        
        self.IDSFfiles = {
            2017: "RunBCDEF_SF_ID.root",
        }


    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        ##Branch with vector of electrons
        self.muonVec = ROOT.std.vector(ROOT.Muon)()
        self.muonBranch = self.out.tree().Branch("muon", self.muonVec)

        if hasattr(inputTree, "GenPart_pt"):
            self.isData = False
            
            ## 2D hist for SF for trigger
            self.triggerSFfile = ROOT.TFile(self.fileDir + self.triggerSFfiles[self.era])
            self.triggerSFhist = self.triggerSFfile.Get("IsoMu27_PtEtaBins/pt_abseta_ratio")

            ## 2D hist for SF for isolation
            self.isoSFfile = ROOT.TFile(self.fileDir + self.isoSFfiles[self.era])
            self.looseIsoMediumSFhist = self.isoSFfile.Get("NUM_LooseRelIso_DEN_MediumID_pt_abseta")
            self.tightIsoMediumSFhist = self.isoSFfile.Get("NUM_TightRelIso_DEN_MediumID_pt_abseta")
            self.looseIsoTightSFhist = self.isoSFfile.Get("NUM_LooseRelIso_DEN_TightIDandIPCut_pt_abseta")
            self.tightIsoTightSFhist = self.isoSFfile.Get("NUM_TightRelIso_DEN_TightIDandIPCut_pt_abseta")

            ## 2D hist for SF for ID
            self.IDSFfile = ROOT.TFile(self.fileDir + self.IDSFfiles[self.era])
            self.mediumIDHist = self.IDSFfile.Get("NUM_MediumID_DEN_genTracks_pt_abseta")
            self.tightIDHist = self.IDSFfile.Get("NUM_TightID_DEN_genTracks_pt_abseta")

            outputFile.cd()


    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass


    def triggerMatching(self, event, muonVec, filterBit):
        trigObject = Collection(event, "TrigObj")

        if filterBit == 2:
            for obj in trigObject:
                if obj.id == 13:
                    matched = [5e-2 > np.sqrt((obj.eta - muon.fourVec.Eta())**2 + (obj.phi - muon.fourVec.Phi())**2) for muon in muonVec]

        return True in matched

                    
    def analyze(self, event):
        ##Clear vectors
        self.muonVec.clear()

        ##Get electrons of Tree
        muons = Collection(event, "Muon")

        ##Loop over jets and set all information
        for muon in muons:
            if muon.pt > self.ptCut and abs(muon.eta) < self.etaCut: 
                validMuon = ROOT.Muon()

                muon4Vec = ROOT.TLorentzVector()
                muon4Vec.SetPtEtaPhiM(muon.pt, muon.eta, muon.phi, 105.658*1e-3)

                validMuon.fourVec = muon4Vec;
                validMuon.isMedium = muon.mediumId;
                validMuon.isTight = muon.tightId;
                validMuon.isLooseIso = muon.pfRelIso04_all < 0.25;
                validMuon.isTightIso = muon.pfRelIso04_all < 0.15;
                validMuon.charge = muon.charge;

                ##Calculate SF for reco and MVA
                if not self.isData:
                    triggerSF = self.triggerSFhist.GetBinContent(self.triggerSFhist.FindBin(muon.pt, abs(muon.eta)))
                    mediumIDSF = self.mediumIDHist.GetBinContent(self.mediumIDHist.FindBin(muon.pt, abs(muon.eta)))
                    tightIDSF = self.tightIDHist.GetBinContent(self.tightIDHist.FindBin(muon.pt, abs(muon.eta)))
                    looseIsoMediumSF = self.looseIsoMediumSFhist.GetBinContent(self.looseIsoMediumSFhist.FindBin(muon.pt, abs(muon.eta)))
                    tightIsoMediumSF = self.tightIsoMediumSFhist.GetBinContent(self.tightIsoMediumSFhist.FindBin(muon.pt, abs(muon.eta)))
                    looseIsoTightSF = self.looseIsoTightSFhist.GetBinContent(self.looseIsoTightSFhist.FindBin(muon.pt, abs(muon.eta)))
                    tightIsoTightSF = self.tightIsoTightSFhist.GetBinContent(self.tightIsoTightSFhist.FindBin(muon.pt, abs(muon.eta)))
                        
                    validMuon.looseIsoMediumSF = looseIsoMediumSF if looseIsoMediumSF!=0 else 1
                    validMuon.tightIsoMediumSF = tightIsoMediumSF if tightIsoMediumSF!=0 else 1
                    validMuon.looseIsoTightSF = looseIsoTightSF if looseIsoTightSF!=0 else 1
                    validMuon.tightIsoTightSF = tightIsoTightSF if tightIsoTightSF!=0 else 1
                    validMuon.mediumSF = mediumIDSF if mediumIDSF!=0 else 1
                    validMuon.tightSF = tightIDSF if tightIDSF!=0 else 1;
                    validMuon.triggerSF = triggerSF;

                self.muonVec.push_back(validMuon)

        if(self.muonVec.size() < self.minNMuon):
            return False

        ##Trigger matching
        if self.triggerBit!=None:
            if not self.triggerMatching(event, self.muonVec, self.triggerBit):
                return False            
     
        ##Fill branches
        self.muonBranch.Fill()

        return True
       
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cHiggsMuonSelector = lambda era, ptCut, etaCut, minNMuon, triggerBit=None: muonSelector(era = era, ptCut = ptCut, etaCut = etaCut, minNMuon=minNMuon, triggerBit = triggerBit) 
