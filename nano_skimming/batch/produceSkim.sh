#!/bin/bash

source /cvmfs/cms.cern.ch/cmsset_default.sh
source /cvmfs/cms.cern.ch/crab3/crab.sh
export SCRAM_ARCH=slc6_amd64_gcc700

eval `scramv1 project CMSSW CMSSW_10_4_0`
cd CMSSW_10_4_0/src/
eval `scramv1 runtime -sh`

mv ../../ChargedHiggs/ ./
mv ../../x509 ./

export X509_USER_PROXY=$CMSSW_BASE/src/x509

scram b

xrdcp $1 $(echo $1 | tr '/' '_')

python ChargedHiggs/nano_skimming/scripts/nanoskimmer.py --filename $(echo $1 | tr '/' '_') --out-name $2 --channel $3 $4
rm $(echo $1 | tr '/' '_')
mv *.root ../../
