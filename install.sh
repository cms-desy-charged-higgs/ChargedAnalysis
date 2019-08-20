#!/bin/bash

##Create analysis dir
mkdir -p ChargedHiggs/
export CHDIR=$(pwd)/ChargedHiggs
cd $CHDIR

##Install anaconda for python2.7 (because of CMSSW)
wget https://repo.anaconda.com/archive/Anaconda2-2019.03-Linux-x86_64.sh
chmod a+rx Anaconda2-2019.03-Linux-x86_64.sh
./Anaconda2-2019.03-Linux-x86_64.sh  ##Set anaconda in $CHDIR/Anaconda
command rm Anaconda2-2019.03-Linux-x86_64.sh

##Install packages with anaconda
source $CHDIR/Anaconda/bin/activate
conda install jupyter numpy tensorflow scipy matplotlib scikit-learn pandas
pip install htcondor
conda deactivate

##Install CMSSW 
source /cvmfs/cms.cern.ch/cmsset_default.sh
source /cvmfs/cms.cern.ch/crab3/crab.sh
export SCRAM_ARCH=slc6_amd64_gcc700

##Add webpage for cern
git clone https://github.com/DaveBrun94/CernWebpage.git

eval `scramv1 project CMSSW CMSSW_9_4_13`
cd CMSSW_9_4_13/src/
eval `scramv1 runtime -sh`
git cms-init

##Clone charged higgs repos
git clone https://github.com/cms-desy-charged-higgs/ChargedAnalysis.git
git clone https://github.com/cms-desy-charged-higgs/ChargedProduction.git
git clone https://github.com/cms-desy-charged-higgs/ChargedNetwork.git

##Other stuff neeeded
git cms-merge-topic cms-egamma:EgammaPostRecoTool
git clone https://github.com/cms-analysis/HiggsAnalysis-CombinedLimit.git HiggsAnalysis/CombinedLimit
git clone https://github.com/cms-analysis/CombineHarvester.git CombineHarvester


##Compile everthing
scram b -j 20

##Set PYTHONPATH
PYTHONPATH=$CHDIR/Anaconda/lib/python2.7/site-packages/:$PYTHONPATH

