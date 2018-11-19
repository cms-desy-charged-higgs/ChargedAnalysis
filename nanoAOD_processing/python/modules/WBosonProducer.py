import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class WBosonProducer(Module):
    def __init__(self, lepton):
        
        self.lepton = lepton

        self.parameter = {
                    "pt": lambda x: x.Pt(), 
                    "phi": lambda x: x.Phi(), 
                    "mT": lambda x: x.Mt(),
        }

    def beginJob(self):
        pass
    def endJob(self):
        pass
    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        for param in self.parameter:
            self.out.branch("WBoson_{}".format(param),  "F")

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        met = electrons = Object(event, "MET")

        lep4vector = ROOT.TLorentzVector()
        met4vector = ROOT.TLorentzVector()

        lep4vector.SetPtEtaPhiM(*[getattr(event, self.lepton + "1_" + parameter) for parameter in ["pt", "eta", "phi", "mass"]])
        met4vector.SetPtEtaPhiM(met.pt, 0, met.phi, 0)

        WBoson4vector = lep4vector + met4vector

        for param_name, param_value in self.parameter.iteritems():
            self.out.fillBranch("WBoson_{}".format(param_name), param_value(WBoson4vector))

        return True
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

ChargedHiggsWBosonProducer = lambda : WBosonProducer(lepton = "electron") 
