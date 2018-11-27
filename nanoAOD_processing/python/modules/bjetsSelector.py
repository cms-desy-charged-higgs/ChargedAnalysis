import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class bjetsSelector(Module):
    def __init__(self, era, csvWP, minNTags, pt_cut):
        #https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation94X

        self.btag = {
            2017: {
                    "loose": 0.1522, 
                    "medium": 0.4941, 
                    "tight": 0.8001,
            }
}

        self.parameter = {
                        "pt": lambda x: x.pt, 
                        "eta": lambda x: x.eta,
                        "phi": lambda x: x.phi, 
                        "mass": lambda x: x.mass,
        }

        self.era = era
        self.csvWP = csvWP
        self.minNTags = minNTags
        self.pt_cut = pt_cut

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        self.out.branch("nbJets",  "I")
        
        for n in range(self.minNTags):
            for param in self.parameter:
                self.out.branch("bJet{}_{}".format(n+1, param),  "F")


    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        if event.nJet<self.minNTags:
            return False

        else:
            jets = Collection(event, "Jet")
            bjets = []

            for jet in jets:
                if jet.btagDeepB > self.btag[self.era][self.csvWP] and jet.pt > self.pt_cut:
                    bjets.append(jet)

            
            if len(bjets) < self.minNTags:
                return False

            else:
                self.out.fillBranch("nbJets", len(bjets))

                for n in range(self.minNTags):
                    for param_name, param_value in self.parameter.iteritems():
                        self.out.fillBranch("bJet{}_{}".format(n+1, param_name), param_value(bjets[n]))

                return True
            
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

ChargedHiggsBjetsSelector = lambda : bjetsSelector(era = 2017, csvWP = "loose", pt_cut = 30, minNTags = 4) 
