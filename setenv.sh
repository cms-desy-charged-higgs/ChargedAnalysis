#!/bin/bash

CURRENTDIR=$PWD

##Reset some variables
export PATH="$(getconf PATH)"
export LD_LIBRARY_PATH=""
export PYTHONPATH=""

##Set analysis dir
CHDIR=$(readlink -f $BASH_SOURCE)
export CHDIR=${CHDIR/"/ChargedAnalysis/setenv.sh"}

##Load grid enviroment
unset DESY_SITE_ENV_SET
unset GLITE_ENV_SET
unset AA_GRID_ENV_SET
source /cvmfs/grid.desy.de/etc/profile.d/grid-ui-env.sh

##Set actual CMSSW version https://twiki.cern.ch/twiki/bin/viewauth/CMS/PdmVAnalysisSummaryTable
# https://twiki.cern.ch/twiki/bin/view/CMS/PdmVRun2LegacyAnalysis
export CMSSW_VERSION=CMSSW_10_6_20

case $1 in
    "CMSSW")
         ##CMSSW related enviroment
        export SCRAM_ARCH="slc7_amd64_gcc700"

        cd $CHDIR/$CMSSW_VERSION/src
        source /cvmfs/cms.cern.ch/cmsset_default.sh
        eval `scramv1 runtime -sh`
        source /cvmfs/cms.cern.ch/crab3/crab.sh ""

        alias scramb="scram b -j 20"

        cd $CURRENTDIR
        ;;
            
    "Analysis")
        export PYTHONPATH=$CHDIR/Anaconda3/lib/python3.8/site-packages/:$CHDIR/ChargedAnalysis/Analysis/python:$CHDIR/ChargedAnalysis/Workflow/python:$CHDIR/ChargedAnalysis/Utility/python:$CHDIR/ChargedAnalysis/Network/python:$PYTHONPATH
        export PATH=$CHDIR/Anaconda3/bin:$CHDIR/ChargedAnalysis/bin:$PATH

        source /cvmfs/sft.cern.ch/lcg/contrib/gcc/9binutils/x86_64-centos7/setup.sh

        alias make="make -j 20"

        ;;

    *)    
        echo "Invalid option: $1"
        echo "Please use this options: {CMSSW, Analysis}"
        return 1
        ;;
esac

##VO related stuff
export X509_USER_PROXY=$HOME/.globus/x509
export REDIRECTOR=root://cms-xrd-global.cern.ch/
export SRM_DESY=srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/cms/tier2/store/user/

alias voms="voms-proxy-init --voms cms:/cms/dcms --valid 168:00"

##CERN stuff
export CERN_USER="dbrunner"
alias www="rsync -r --update --existing --exclude '.git*' -e 'ssh' $CHDIR/CernWebpage/ $CERN_USER@lxplus.cern.ch:/eos/home-${USER:0:1}/$CERN_USER/www/"
