#!/bin/bash

export HOME=./

source /cvmfs/cms.cern.ch/cmsset_default.sh
export SCRAM_ARCH=slc6_amd64_gcc630

eval `scramv1 project CMSSW CMSSW_10_2_11`
cd CMSSW_10_2_11/src/
eval `scramv1 runtime -sh`

mkdir -p Configuration/GenProduction/python

mv ../../$1 ./Configuration/GenProduction/python
mv ../../$2 ./
mv ../../x509 ./

export X509_USER_PROXY=$CMSSW_BASE/src/x509

scram b

export NAME=$3
export CONDITIONS=102X_upgrade2018_realistic_v16
export ERA=Run2_2018
export N=2000

cmsDriver.py LHEtoEDM --filein file:${NAME}.lhe --fileout file:${NAME}_pLHE.root --mc --eventcontent LHE --datatier LHE --conditions $CONDITIONS --era $ERA --step NONE --python_filename ${NAME}_pLHE_cfg.py --customise Configuration/DataProcessing/Utils.addMonitoring -n $N 

cmsDriver.py Configuration/GenProduction/python/cHiggsfragment.py --filein file:${NAME}_pLHE.root --fileout file:${NAME}_GS.root --mc --eventcontent RAWSIM --datatier GEN-SIM --conditions $CONDITIONS --beamspot Realistic25ns13TeVEarly2017Collision --step GEN,SIM --nThreads 1 --geometry DB:Extended --era $ERA --python_filename ${NAME}_GS_cfg.py --customise Configuration/DataProcessing/Utils.addMonitoring -n $N

cmsDriver.py DIGI --filein file:${NAME}_GS.root --fileout file:file:${NAME}_DRP.root --pileup_input "dbs: /Neutrino_E-10_gun/RunIISummer17PrePremix-PUAutumn18_102X_upgrade2018_realistic_v15-v1/GEN-SIM-DIGI-RAW" --mc --eventcontent PREMIXRAW --datatier GEN-SIM-RAW --conditions $CONDITIONS --step DIGI,DATAMIX,L1,DIGI2RAW,HLT:@relval2018 --datamix PreMix --era $ERA --python_filename ${NAME}_DRP_cfg.py --customise Configuration/DataProcessing/Utils.addMonitoring -n $N --geometry DB:Extended

cmsDriver.py AOD --mc --eventcontent AODSIM runUnscheduled --python_filename ${NAME}_AOD_cfg.py --datatier AODSIM --conditions $CONDITIONS --step RAW2DIGI,RECO,EI --era $ERA --filein file:${NAME}_DRP.root --fileout file:${NAME}_AOD.root -n $N --geometry DB:Extended

cmsDriver.py MINIAOD --mc --python_filename ${NAME}_MINIAOD_cfg.py --eventcontent MINIAODSIM --runUnscheduled --datatier MINIAODSIM --conditions $CONDITIONS --step PAT --era $ERA  --filein file:${NAME}_AOD.root --fileout file:${NAME}_MINIAOD.root -n $N

cmsDriver.py NANOAOD --mc --eventcontent NANOAODSIM --python_filename ${NAME}_NANOAOD_cfg.py --datatier NANOAODSIM --conditions $CONDITIONS --era $ERA,run2_nanoAOD_102Xv1 --step NANO --filein file:${NAME}_MINIAOD.root --fileout file:${NAME}_NANOAOD.root -n $N --customise_commands 'process.particleLevelSequence.remove(process.genParticles2HepMCHiggsVtx);process.particleLevelSequence.remove(process.rivetProducerHTXS);process.particleLevelTables.remove(process.HTXSCategoryTable)'

mv ${NAME}_MINIAOD.root ../../
mv ${NAME}_NANOAOD.root ../../
