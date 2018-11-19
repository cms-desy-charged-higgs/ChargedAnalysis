import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class electronSelector(Module):
    def __init__(self, era, ID_wp, pt_cut, eta_cut, minN):

        self.mva_ID = {
                    2017: {
                            "loose": lambda x: x.mvaFall17Iso_WPL,  
                            "WP80": lambda x: x.mvaFall17Iso_WP80, 
                            "WP90": lambda x: x.mvaFall17Iso_WP90,
                    }
        }
        
        self.parameter = {
                        "pt": lambda x: x.pt, 
                        "eta": lambda x: x.eta,
                        "phi": lambda x: x.phi, 
                        "mass": lambda x: x.mass,
        }

        self.era = era
        self.pt_cut = pt_cut
        self.eta_cut = pt_cut

        self.ID_wp = ID_wp
        self.minN = minN

    def beginJob(self):
        pass
    def endJob(self):
        pass
    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        self.out.branch("nElectron",  "I")
        
        for n in range(self.minN):
            for param in self.parameter:
                self.out.branch("electron{}_{}".format(n+1, param),  "F")


    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass
    def analyze(self, event):
        if event.nElectron<self.minN:
            return False

        else:
            electrons = Collection(event, "Electron")
            electrons_cand = []

            for electron in electrons:
                if self.mva_ID[self.era][self.ID_wp](electron) and electron.pt > self.pt_cut and electron.eta < self.eta_cut: 
                    electrons_cand.append(electron)

            
            if len(electrons_cand) < self.minN:
                return False

            else:
                self.out.fillBranch("nElectron", len(electrons_cand))

                for n in range(self.minN):
                    for param_name, param_value in self.parameter.iteritems():
                        self.out.fillBranch("electron{}_{}".format(n+1, param_name),  param_value(electrons_cand[n]))

                return True
            
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

ChargedHiggsElectronSelector = lambda : electronSelector(era = 2017, ID_wp = "WP80", pt_cut = 28.0, eta_cut = 2.4, minN = 1) 
