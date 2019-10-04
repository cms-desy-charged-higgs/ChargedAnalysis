#!/bin/bash

export PATH=$(getconf PATH)
export LD_LIBRARY_PATH=""

##Set analysis dir
CHDIR=$(readlink -f $BASH_SOURCE)
export CHDIR=${CHDIR/"/ChargedAnalysis/setenv.sh"}

##Set actual CMSSW version
export CMSSW_VERSION=CMSSW_9_4_13

case $1 in
    "CMSSW")
         ##CMSSW related enviroment
        export SCRAM_ARCH="slc6_amd64_gcc700"

        cd $CHDIR/$CMSSW_VERSION/src
        source /cvmfs/cms.cern.ch/cmsset_default.sh
        eval `scramv1 runtime -sh`
        source /cvmfs/cms.cern.ch/crab3/crab.sh

        alias scramb="scram b -j 20"

        cd $CHDIR
        ;;
            
    "StandAlone")
        export PYTHONPATH=$CHDIR/Anaconda3/lib/python3.7/site-packages/:$CHDIR/ChargedAnalysis/Analysis/python:$CHDIR/ChargedAnalysis/Workflow/python:$CHDIR/ChargedNetwork/python
        export PATH=$CHDIR/Anaconda3/bin:$CHDIR/ChargedAnalysis/Analysis/bin:$CHDIR/ChargedAnalysis/Workflow/bin:$CHDIR/ChargedNetwork/bin:$PATH
        export LD_LIBRARY_PATH=$CHDIR/Anaconda3/lib:$CHDIR/ChargedAnalysis/Analysis/lib

        alias make="make -j 20"

        ;;

    *)    
        echo "Invalid option: $1"
        echo "Please use this options: {CMSSW, StandAlone}"
        return 1
        ;;
esac

##VO related stuff
export X509_USER_PROXY=$HOME/.globus/x509
export REDIRECTOR=root://cms-xrd-global.cern.ch/
export SRM_DESY=srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/cms/tier2/store/user/

alias voms="voms-proxy-init --voms cms --valid 168:00"

##CERN stuff
export CERN_USER="dbrunner"
alias www="rsync -arvt --exclude '.*' --delete -e ssh $CHDIR/CernWebpage/ $CERN_USER@lxplus.cern.ch:/eos/home-${USER:0:1}/$CERN_USER/www/"
