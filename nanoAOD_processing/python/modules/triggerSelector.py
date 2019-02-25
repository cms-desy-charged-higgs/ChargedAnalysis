import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

from PhysicsTools.NanoAODTools.postprocessing.framework.datamodel import Collection, Object 
from PhysicsTools.NanoAODTools.postprocessing.framework.eventloop import Module


class triggerSelector(Module):
    def __init__(self, pathToPass):
        self.pathToPass = pathToPass

    def beginJob(self):
        pass

    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def endFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        pass

    def analyze(self, event):
        passedPaths = [getattr(event, trigger) for trigger in self.pathToPass]

        if False in passedPaths:
            return False

        #if True in notPassedPaths:
         #   return False

        return True
            
# define modules using the syntax 'name = lambda : constructor' to avoid having them loaded when not needed

cHiggsTriggerSelector = lambda pathToPass: triggerSelector(pathToPass)
