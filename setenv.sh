#!/bin/bash

export PATH="$(getconf PATH):/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/cvmfs/grid.cern.ch/centos7-ui-4.0.3-1_umd4v4/usr/bin:/cvmfs/grid.cern.ch/centos7-ui-4.0.3-1_umd4v4/usr/sbin:/opt/puppetlabs/bin"
export LD_LIBRARY_PATH="/cvmfs/grid.cern.ch/centos7-ui-4.0.3-1_umd4v4/lib64:/cvmfs/grid.cern.ch/centos7-ui-4.0.3-1_umd4v4/lib:/cvmfs/grid.cern.ch/centos7-ui-4.0.3-1_umd4v4/usr/lib64:/cvmfs/grid.cern.ch/centos7-ui-4.0.3-1_umd4v4/usr/lib:/cvmfs/grid.cern.ch/centos7-ui-4.0.3-1_umd4v4/usr/lib64/dcap"

export PYTHONPATH=""

##Set analysis dir
CHDIR=$(readlink -f $BASH_SOURCE)
export CHDIR=${CHDIR/"/ChargedAnalysis/setenv.sh"}

##Set actual CMSSW version https://twiki.cern.ch/twiki/bin/viewauth/CMS/PdmVAnalysisSummaryTable
export CMSSW_VERSION=CMSSW_10_2_18

case $1 in
    "CMSSW")
         ##CMSSW related enviroment
        export SCRAM_ARCH="slc7_amd64_gcc700"

        cd $CHDIR/$CMSSW_VERSION/src
        source /cvmfs/cms.cern.ch/cmsset_default.sh
        eval `scramv1 runtime -sh`
        source /cvmfs/cms.cern.ch/crab3/crab.sh

        alias scramb="scram b -j 20"

        cd $CHDIR
        ;;
            
    "Analysis")
        export PYTHONPATH=$CHDIR/Anaconda3/lib/python3.7/site-packages/:$CHDIR/ChargedAnalysis/Analysis/python:$CHDIR/ChargedAnalysis/Workflow/python:$CHDIR/ChargedAnalysis/Utility/python:$CHDIR/ChargedAnalysis/Network/python:$CHDIR/HiggsAnalysis/CombinedLimit/lib/python
        export PATH=$CHDIR/Anaconda3/bin:$CHDIR/ChargedAnalysis/Analysis/bin:$CHDIR/ChargedAnalysis/Workflow/bin:$CHDIR/ChargedAnalysis/Network/bin:$CHDIR/ChargedAnalysis/Utility/bin:$CHDIR/HiggsAnalysis/CombinedLimit/exe:$CHDIR/HiggsAnalysis/CombinedLimit/scripts:$PATH
        export LD_LIBRARY_PATH=$CHDIR/Anaconda3/lib:$CHDIR/ChargedAnalysis/Analysis/lib:$CHDIR/ChargedAnalysis/Network/lib:$CHDIR/ChargedAnalysis/Utility/lib:$CHDIR/Anaconda3/lib/python3.7/site-packages/torch/lib:$CHDIR/HiggsAnalysis/CombinedLimit/lib:$LD_LIBRARY_PATH

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
alias www="rsync -art --exclude '.git*' --delete -e ssh $CHDIR/CernWebpage/ $CERN_USER@lxplus.cern.ch:/eos/home-${USER:0:1}/$CERN_USER/www/"
