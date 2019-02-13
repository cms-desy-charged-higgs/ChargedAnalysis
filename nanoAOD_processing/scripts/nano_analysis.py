#!/usr/bin/env python

import os
import argparse
import sys
import subprocess
import time

sys.path.append("/usr/lib64/python2.6/site-packages/")
import htcondor

import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True
from dbs.apis.dbsClient import DbsApi

from PhysicsTools.NanoAODTools.postprocessing.framework.postprocessor import PostProcessor
from ChargedHiggs.nanoAOD_processing.chargedHiggsModules import *


def parser():
    parser = argparse.ArgumentParser(description = "Script to skim NANOAOD root files", formatter_class=argparse.RawTextHelpFormatter)
    
    parser.add_argument("--filename", action = "store", help = "Name of the input NANOAOD root file")
    parser.add_argument("--channel", action = "store", required = True, choices = ["e4b", "m4b"], help = "Final state which is of interest")
    parser.add_argument("--bkg-txt", action = "store", help = "Txt file with data set names of bkg samples")
    parser.add_argument("--data-txt", action = "store", help = "Txt file with data set names of data samples")
    parser.add_argument("--sig-txt", action = "store", help = "Txt file with SE path to personal dCache signal dir")

    parser.add_argument("--batch", type = str, choices = ["local", "condor"], default = "local", help = "State if you want local or HTcondor jobs")
    parser.add_argument("--out-dir", type = str, default = "{}/src".format(os.environ["CMSSW_BASE"]), help = "Name of output directory")

    return parser.parse_args()

def get_filenames(bkgTXT, dataTXT, sigTXT):
    ##dasgoclient python API
    dbs = DbsApi('https://cmsweb.cern.ch/dbs/prod/global/DBSReader')
    global_director = "root://cmsxrootd.fnal.gov/"

    ##Read out input files containing data set names
    if bkgTXT:
        with open(bkgTXT) as f:
            background = [background for background in f.read().splitlines() if background != ""]

    else:
        background = []

    if dataTXT:
        with open(dataTXT) as f:
            data = [data for data in f.read().splitlines() if data != ""]

    else:
        data = []

    if sigTXT:
        with open(sigTXT) as f:
            signal = [signal for signal in f.read().splitlines() if signal != ""]

    else:
        signal = []

    ##Fill file names in using dasgoclient API
    filelist = {}

    for setname in background + data:
        if "mc" in setname:
            key = setname.split("/")[1]
            
        else:
            key = setname.split("/")[1] + "-" +  setname.split("/")[2]

        filelist[key] = [global_director + filename['logical_file_name'] for filename in dbs.listFiles(dataset=setname, detail=1)]

    ##Read out signal files with gfal-ls command
    for SEpath in signal:
        key = SEpath.split("/")[-2]
        signalFiles = subprocess.check_output(["gfal-ls", SEpath]).split("\n")[:-1]

        filelist[key] = [global_director + SEpath[74:] + "/" + signalFile for signalFile in signalFiles]
    
    return filelist

def post_processor(out_dir, filelist, channel):
    ##NanoAOD postprocessor class
    p = PostProcessor(
                    outputDir=".", 
                    inputFiles= filelist,
                    noOut=False,
                    modules= module_list[channel],
                    outputbranchsel="{}/src/ChargedHiggs/nanoAOD_processing/data/keep_and_drop.txt".format(os.environ["CMSSW_BASE"]),                                        
    )

    p.run()


def condor_submit(outdir, filename, channel):
    ##Condor class    
    job = htcondor.Submit()
    schedd = htcondor.Schedd()

    skimfilename = "_".join(filename.split("/"))[:-5] + "_Skim.root"

    ##Condor configuration
    job["executable"] = "{}/src/ChargedHiggs/nanoAOD_processing/batch/condor_script.sh".format(os.environ["CMSSW_BASE"])
    job["arguments"] = " ".join([filename, channel])
    job["universe"]       = "vanilla"

    job["should_transfer_files"] = "YES"
    job["transfer_input_files"]       = ",".join([os.environ["CMSSW_BASE"] + "/src/ChargedHiggs", os.environ["CMSSW_BASE"] + "/src/x509"])

    job["log"]                    = outdir + "/log/job_$(Cluster).log"
    job["output"]                    = outdir + "/log/job_$(Cluster).out"
    job["error"]                    = outdir + "/log/job_$(Cluster).err"

    #job["+RequestRuntime"]    = "{}".format(60*60*12)
    job["when_to_transfer_output"] = "ON_EXIT"
    job["transfer_output_remaps"] = '"' + '{filename} = {outdir}/{filename}'.format(filename=skimfilename, outdir=outdir) + '"'

    ##Agressively submit your jobs
    def submit(schedd, job):
        with schedd.transaction() as txn:
            job.queue(txn)
            print "Submit job for file {}".format(filename)
          

    while(True):
        try: 
            submit(schedd, job)
            break    

        except:
            pass

def main():
    args = parser()

    ##Call local with --filename argument
    if args.batch == "local":
        post_processor(args.out_dir, [args.filename], args.channel)

    ##Call batch with txt files input
    elif args.batch == "condor":
        filelist = get_filenames(args.bkg_txt, args.data_txt, args.sig_txt)

        skimdir = "/nfs/dust/cms/user/{}/Skimdir/nanoSkim_{}".format(os.environ["USER"], "_".join([str(time.localtime()[i]) for i in range(6)]))

        ##Create dirs and copy proxy file to initialdir
        os.system("mkdir -p {}".format(skimdir))        
        os.system("chmod 755 {}".format(os.environ["X509_USER_PROXY"])) 
        os.system("cp -u {} {}/src/".format(os.environ["X509_USER_PROXY"], os.environ["CMSSW_BASE"])) 

        ##Submit all the things
        for dirname, filenames in filelist.iteritems():
            outdir = skimdir + "/" + dirname

            os.system("mkdir -p {}/log".format(outdir))

            for filename in filenames:
                condor_submit(outdir, filename, args.channel)
       
   
if __name__ == "__main__":
    main()
