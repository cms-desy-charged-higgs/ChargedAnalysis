from WMCore.Configuration import Configuration
from CRABClient.UserUtilities import config, getUsernameFromSiteDB

import os

config = Configuration()

config.section_("General")
config.General.requestName = 'NanoPost1'
config.General.transferLogs=True


config.section_("JobType")
config.JobType.pluginName = 'Analysis'
config.JobType.psetName = '%s/src/ChargedHiggs/nanoAOD_processing/batch/PSet.py' % (os.environ["CMSSW_BASE"])
config.JobType.scriptExe = '%s/src/ChargedHiggs/nanoAOD_processing/batch/crab_script.sh' % (os.environ["CMSSW_BASE"])
config.JobType.inputFiles = ['%s/src/ChargedHiggs/nanoAOD_processing/batch/crab_script.py' % (os.environ["CMSSW_BASE"])]
config.JobType.sendPythonFolder	 = True



config.section_("Data")
config.Data.inputDataset = '/DYJetsToLL_1J_TuneCP5_13TeV-amcatnloFXFX-pythia8/RunIIFall17NanoAOD-PU2017_12Apr2018_94X_mc2017_realistic_v14-v1/NANOAODSIM'
config.Data.inputDBS = 'global'
config.Data.splitting = 'FileBased'
config.Data.unitsPerJob = 2
config.Data.totalUnits = 10
config.Data.outLFNDirBase = '/store/user/%s/NanoPost' % (getUsernameFromSiteDB())
config.Data.publication = False
config.Data.outputDatasetTag = 'NanoPost'

config.section_("Site")
config.Site.storageSite = "T2_DE_DESY"
