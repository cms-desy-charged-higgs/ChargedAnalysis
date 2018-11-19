import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class ChargedHiggsProducer(Module):
    def __init__(self, lepton):
        
        self.lepton = lepton

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

        for param in self.parameter:
            self.out.branch("chargedHiggs_{}".format(param),  "F")

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        met = electrons = Object(event, "MET")

        lep4vector = ROOT.TLorentzVector()
        sHiggs14vector = ROOT.TLorentzVector()
        sHiggs24vector = ROOT.TLorentzVector()

        lep4vector.SetPtEtaPhiM(*[getattr(event, self.lepton + "1_" + parameter) for parameter in ["pt", "eta", "phi", "mass"]])
        sHiggs14vector.SetPtEtaPhiM(*[getattr(event, "smallHiggs1_" + parameter) for parameter in ["pt", "eta", "phi", "mass"]])
        sHiggs24vector.SetPtEtaPhiM(*[getattr(event, "smallHiggs2_" + parameter) for parameter in ["pt", "eta", "phi", "mass"]])
        
        chargedHiggs4vector = lep4vector + sHiggs14vector + sHiggs24vector

        for param_name, param_value in self.parameter.iteritems():
            self.out.fillBranch("chargedHiggs_{}".format(param_name), param_value(chargedHiggs4vector))

        return True
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

ChargedHiggsBosonProducer = lambda : ChargedHiggsProducer(lepton = "electron") 
