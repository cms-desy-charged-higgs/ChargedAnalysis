#!/bin/bash

##Create analysis dir
mkdir -p ChargedHiggs/
export CHDIR=$(pwd)/ChargedHiggs
cd $CHDIR

source /cvmfs/sft.cern.ch/lcg/contrib/gcc/9binutils/x86_64-centos7/setup.sh

##Install Anaconda
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
chmod +x Miniconda3-latest-Linux-x86_64.sh
./Miniconda3-latest-Linux-x86_64.sh -b -p $CHDIR/Anaconda3
command rm Miniconda3-latest-Linux-x86_64.sh

##Install necessary things with anaconda
export PYTHONPATH=$CHDIR/Anaconda3/lib/python3.7/site-packages/
source $CHDIR/Anaconda3/bin/activate

##Install pytorch
conda install pytorch torchvision cudatoolkit=10.1 -c pytorch-nightly -y

conda install -c anaconda git make pyyaml cmake pandas -y
conda install -c conda-forge boost vdt root eigen -y

##Git standalone analysis code and compile everything
cd $CHDIR
git clone https://github.com/cms-desy-charged-higgs/ChargedAnalysis.git

cd ChargedAnalysis
make -j 20
cd ..

##Add webpage for cern
git clone https://github.com/DaveBrun94/CernWebpage.git

conda deactivate

##Install CMSSW 
source /cvmfs/cms.cern.ch/cmsset_default.sh
source /cvmfs/cms.cern.ch/crab3/crab.sh
export SCRAM_ARCH=slc7_amd64_gcc700

eval `scramv1 project CMSSW CMSSW_10_6_20`
cd CMSSW_10_6_20/src/
eval `scramv1 runtime -sh`
git cms-init

##Needed for electron because people fucked up
git cms-addpkg RecoEgamma/EgammaTools 
mv EgammaPostRecoTools/python/EgammaPostRecoTools.py RecoEgamma/EgammaTools/python/
git clone https://github.com/cms-data/EgammaAnalysis-ElectronTools.git EgammaAnalysis/ElectronTools/data/
git cms-addpkg EgammaAnalysis/ElectronTools

git clone https://github.com/cms-analysis/HiggsAnalysis-CombinedLimit.git HiggsAnalysis/CombinedLimit
git clone https://github.com/cms-analysis/CombineHarvester.git CombineHarvester

##Clone charged higgs repos
git clone https://github.com/cms-desy-charged-higgs/ChargedSkimming.git
git clone https://github.com/cms-desy-charged-higgs/ChargedProduction.git

##Compile everthing
scram b -j 20

cd $CHDIR
