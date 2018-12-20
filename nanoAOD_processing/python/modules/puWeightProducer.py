from PhysicsTools.NanoAODTools.postprocessing.modules.common.puWeightProducer import puWeightProducer

import os

class puWeightProducerModified(puWeightProducer):
    def __init__(self, myfile, targetfile):
        self.isData = True
        
        puWeightProducer.__init__(self,myfile,targetfile,myhist="pu_mc",targethist="pileup",name="puWeight",norm=True,verbose=False,nvtx_var="Pileup_nTrueInt",doSysVar=True)

    def beginJob(self):
        pass
        
    def endJob(self):
        pass

    def beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree):
        if "RunIIFall" in inputFile.GetName():
            self.isData = False

            puWeightProducer.beginFile(self, inputFile, outputFile, inputTree, wrappedOutputTree)

    def analyze(self, event):
        if not self.isData:
            puWeightProducer.analyze(self, event)

        return True

pufile_basedir = "%s/src/PhysicsTools/NanoAODTools/python/postprocessing/data/pileup" % os.environ['CMSSW_BASE']

pufile_data2017=os.path.join(pufile_basedir, "pileup_Cert_294927-306462_13TeV_PromptReco_Collisions17_withVar.root")
puAutoWeight = lambda : puWeightProducerModified("auto",pufile_data2017)

