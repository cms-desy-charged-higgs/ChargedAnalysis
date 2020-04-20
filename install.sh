#!/bin/bash

##Create analysis dir
mkdir -p ChargedHiggs/
export CHDIR=$(pwd)/ChargedHiggs
cd $CHDIR

##Install Anaconda
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
chmod +x Miniconda3-latest-Linux-x86_64.sh
./Miniconda3-latest-Linux-x86_64.sh -b -p $CHDIR/Anaconda3
command rm Miniconda3-latest-Linux-x86_64.sh

##Install ROOT, python packages and g++ compiler with conda
export PYTHONPATH=$CHDIR/Anaconda3/lib/python3.7/site-packages/
source $CHDIR/Anaconda3/bin/activate

conda install -c conda-forge root boost vdt psutil -y
conda install -c anaconda git make pytorch-gpu pyyaml -y

##Source compiler from CERN software package
source /cvmfs/sft.cern.ch/lcg/contrib/gcc/9binutils/x86_64-centos7/setup.sh

##Git standalone analysis code and compile everything
git clone https://github.com/cms-desy-charged-higgs/ChargedAnalysis.git

cd ChargedAnalysis
make -j 20
cd ..

##Install Higgs combine fork
git clone https://github.com/cms-desy-charged-higgs/HiggsAnalysis-CombinedLimit.git HiggsAnalysis/CombinedLimit
cd HiggsAnalysis/CombinedLimit
 
git remote add original https://github.com/cms-desy-charged-higgs/HiggsAnalysis-CombinedLimit.git
git pull original 102x

make -j 20

cd $CHDIR

##Add webpage for cern
git clone https://github.com/DaveBrun94/CernWebpage.git

conda deactivate

##Install CMSSW 
source /cvmfs/cms.cern.ch/cmsset_default.sh
source /cvmfs/cms.cern.ch/crab3/crab.sh
export SCRAM_ARCH=slc7_amd64_gcc700

eval `scramv1 project CMSSW CMSSW_10_2_18`
cd CMSSW_10_2_18/src/
eval `scramv1 runtime -sh`
git cms-init

##Clone charged higgs repos
git clone https://github.com/cms-desy-charged-higgs/ChargedSkimming.git
git clone https://github.com/cms-desy-charged-higgs/ChargedProduction.git

##Other stuff neeeded
git cms-merge-topic cms-egamma:EgammaPostRecoTools

##Compile everthing
scram b -j 20
