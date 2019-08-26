#!/bin/bash

export CMSSW_VERSION=CMSSW_9_4_13

##Set analysis dir
CHDIR=$(readlink -f $BASH_SOURCE)
export CHDIR=${CHDIR/"$CMSSW_VERSION/src/ChargedAnalysis/setenv.sh"}

##CMSSW related enviroment
export SCRAM_ARCH="slc6_amd64_gcc700"

cd $CHDIR/$CMSSW_VERSION/src
source /cvmfs/cms.cern.ch/cmsset_default.sh
cmsenv
source /cvmfs/cms.cern.ch/crab3/crab.sh

alias scramb="scram b -j 20"

##VO related stuff
export X509_USER_PROXY=$HOME/.globus/x509
export REDIRECTOR=root://cms-xrd-global.cern.ch/
export SRM_DESY=srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/cms/tier2/store/user/

alias voms="voms-proxy-init --voms cms --valid 168:00"

##CERN stuff
export CERN_USER="dbrunner"
alias www="rsync -arvt --exclude '.*' --delete -e ssh $CHDIR/CernWebpage/ $CERN_USER@lxplus.cern.ch:/eos/home-${USER:0:1}/$CERN_USER/www/"

##Put Anaconda into PYTHONPATH
PYTHONPATH=$CHDIR/Anaconda/lib/python2.7/site-packages/:$PYTHONPATH
