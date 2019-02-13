#!/bin/bash

source /cvmfs/cms.cern.ch/cmsset_default.sh
source /cvmfs/cms.cern.ch/crab3/crab.sh
export SCRAM_ARCH=slc6_amd64_gcc630

eval `scramv1 project CMSSW CMSSW_9_4_11_patch1`
cd CMSSW_9_4_11_patch1/src/
eval `scramv1 runtime -sh`

mv ../../ChargedHiggs/ ./
mv ../../x509 ./

export X509_USER_PROXY=$CMSSW_BASE/src/x509

git clone https://github.com/cms-nanoAOD/nanoAOD-tools.git PhysicsTools/NanoAODTools

scram b

xrdcp $1 $(echo $1 | tr '/' '_')

python ChargedHiggs/nanoAOD_processing/scripts/nano_analysis.py --filename $(echo $1 | tr '/' '_') --channel $2
mv *_Skim.root ../../

