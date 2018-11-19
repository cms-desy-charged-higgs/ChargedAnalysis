import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class triggerSelector(Module):
    def __init__(self, triggerpaths):
        
        self.triggerpaths = triggerpaths

    def beginJob(self):
        pass
    def endJob(self):
        pass
    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        self.out = wrappedOutputTree

        for trigger in self.triggerpaths:
            self.out.branch(trigger,  "B")

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        trigger_decision = [getattr(event, trigger) for trigger in self.triggerpaths]

        if True in trigger_decision:
            for n, trigger in enumerate(self.triggerpaths):
                self.out.fillBranch(trigger,  trigger_decision[n])
        
            return True

        else:
            return False
            
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

ChargedHiggsEleTriggerSelector = lambda : triggerSelector(triggerpaths = ["HLT_Ele27_WPTight_Gsf"]) 
