import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module

class smallHiggsProducer(Module):
    def __init__(self, nMotherparticles, nDautherparticles, daughtertype):
        self.nMotherparticles = nMotherparticles
        self.nDautherparticles = nDautherparticles
        self.daughtertype = daughtertype

        self.parameter = {
                    "pt": lambda x: x.Pt(), 
                    "eta": lambda x: x.Eta(),
                    "phi": lambda x: x.Phi(), 
                    "mass": lambda x: x.M(),
        }

    def beginJob(self):
        pass
    def endJob(self):
        pass
    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        for n in range(self.nMotherparticles):
            for param in self.parameter:
                self.out.branch("smallHiggs{}_{}".format(n+1, param),  "F")

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass
    def analyze(self, event):
        daughterparticles = []

        for n in range(self.nDautherparticles):
            daughterparticle = ROOT.TLorentzVector()
            daughterparticle.SetPtEtaPhiM(*[getattr(event, self.daughtertype + "{}_".format(n+1) + parameter) for parameter in ["pt", "eta", "phi", "mass"]])

            daughterparticles.append(daughterparticle)

        sHiggspair = []
        deltaMH1H2 = []

        index_min = lambda x: [index for (index, value) in enumerate(x) if value == min(x)][0]
    
        for (n, m, k, l) in [(0,1,2,3), (0,2,1,3), (0,3,1,2)]:
            sHiggs1 = daughterparticles[n] + daughterparticles[m]
            sHiggs2 = daughterparticles[k] + daughterparticles[l]

            sHiggspair.append((sHiggs1, sHiggs2))
            deltaMH1H2.append(abs(sHiggs1.M() - sHiggs2.M()))

        index_min = [index for (index, value) in enumerate(deltaMH1H2) if value == min(deltaMH1H2)][0]

        for param_name, param_value in self.parameter.iteritems():
            sHiggs1, sHiggs2 = sHiggspair[index_min]

            if sHiggs1.Pt() > sHiggs2.Pt():
                self.out.fillBranch("smallHiggs1_{}".format(param_name),  param_value(sHiggs1))
                self.out.fillBranch("smallHiggs2_{}".format(param_name),  param_value(sHiggs2))

            else:
                self.out.fillBranch("smallHiggs1_{}".format(param_name),  param_value(sHiggs2))
                self.out.fillBranch("smallHiggs2_{}".format(param_name),  param_value(sHiggs1))
                

        return True

 
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

ChargedHiggsSmallHiggsProducer = lambda : smallHiggsProducer(nMotherparticles=2, nDautherparticles=4, daughtertype="bJet") 
