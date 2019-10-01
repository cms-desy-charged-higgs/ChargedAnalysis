#!/bin/bash

##Create analysis dir
mkdir -p ChargedHiggs/
export CHDIR=$(pwd)/ChargedHiggs
cd $CHDIR

##Install Anaconda
wget https://repo.anaconda.com/archive/Anaconda3-2019.07-Linux-x86_64.sh
chmod +x Anaconda3-2019.07-Linux-x86_64.sh
./Anaconda3-2019.07-Linux-x86_64.sh
command rm Anaconda3-2019.07-Linux-x86_64.sh

##Install ROOT, python packages and g++ compiler with conda
export PYTHONPATH=$CHDIR/Anaconda3/lib/python3.7/site-packages/
source $CHDIR/Anaconda3/bin/activate

conda install -c conda-forge root -y
conda install jupyter numpy tensorflow-gpu scipy scikit-learn pandas -y
pip install htcondor

conda install -c conda-forge compilers -y
conda install -c anaconda git -y

##Git standalone analysis code and compile everything
git clone https://github.com/cms-desy-charged-higgs/ChargedAnalysis.git
git clone https://github.com/cms-desy-charged-higgs/ChargedNetwork.git

cd ChargedAnalysis
make -j 20
cd ..

##Add webpage for cern
git clone https://github.com/DaveBrun94/CernWebpage.git

conda deactivate

##Install CMSSW 
source /cvmfs/cms.cern.ch/cmsset_default.sh
source /cvmfs/cms.cern.ch/crab3/crab.sh
export SCRAM_ARCH=slc6_amd64_gcc700

eval `scramv1 project CMSSW CMSSW_9_4_13`
cd CMSSW_9_4_13/src/
eval `scramv1 runtime -sh`
git cms-init

##Clone charged higgs repos
git clone https://github.com/cms-desy-charged-higgs/ChargedSkimming.git
git clone https://github.com/cms-desy-charged-higgs/ChargedProduction.git

##Other stuff neeeded
git cms-merge-topic cms-egamma:EgammaPostRecoTools
git clone https://github.com/cms-analysis/HiggsAnalysis-CombinedLimit.git HiggsAnalysis/CombinedLimit

##Compile everthing
scram b -j 20
