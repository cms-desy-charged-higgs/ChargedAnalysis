#!/usr/bin/env python
import os

import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True
from importlib import import_module
from PhysicsTools.NanoAODTools.postprocessing.framework.postprocessor import PostProcessor
from PhysicsTools.NanoAODTools.postprocessing.framework.crabhelper import inputFiles, runsAndLumis

from ChargedHiggs.nanoAOD_processing.chargedHiggsModules import *

p=PostProcessor(
                outputDir=".", 
                inputFiles= inputFiles(),
                noOut=False,
                modules= module_list,
                outputbranchsel="{}/src/ChargedHiggs/nanoAOD_processing/keep_and_drop.txt".format(os.environ["CMSSW_BASE"]                                          
                provenance=True,
                fwkJobReport=True,
                jsonInput=runsAndLumis()),
)

p.run()
