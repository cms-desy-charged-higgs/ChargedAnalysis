#!/usr/bin/env python

import os
import argparse
import time
import sys

sys.path.append("/usr/lib64/python2.6/site-packages/")
import htcondor

import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True
from dbs.apis.dbsClient import DbsApi

from PhysicsTools.NanoAODTools.postprocessing.framework.postprocessor import PostProcessor
from ChargedHiggs.nanoAOD_processing.chargedHiggsModules import *


def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--analysis", type = str, choices = ["ChargedHiggs"], default = "ChargedHiggs")
    parser.add_argument("--filename", type = str)
    parser.add_argument("--filetxt", type = str)

    parser.add_argument("--batch", type = str, choices = ["local", "condor"], default = "local")
    parser.add_argument("--out-dir", type = str, default = "{}/src".format(os.environ["CMSSW_BASE"]))

    return parser.parse_args()

def get_filenames(filetxt):
    dbs = DbsApi('https://cmsweb.cern.ch/dbs/prod/global/DBSReader')

    global_director = "root://cmsxrootd.fnal.gov/"

    with open(filetxt) as f:
        datasets = [dataset for dataset in f.read().splitlines() if dataset != ""]

    filelist = {}

    for dataset in datasets:
        filelist[dataset.split("/")[1]] = [global_director + filename['logical_file_name'] for filename in dbs.listFiles(dataset=dataset)]

    return filelist

def post_processor(out_dir, filelist):

    p = PostProcessor(
                    outputDir=".", 
                    inputFiles= filelist,
                    noOut=False,
                    modules= module_list,
                    outputbranchsel="{}/src/ChargedHiggs/nanoAOD_processing/data/keep_and_drop.txt".format(os.environ["CMSSW_BASE"]),                                        
    )

    p.run()


def condor_submit(outdir, filename):
    job = htcondor.Submit()
    schedd = htcondor.Schedd()

    skimfilename = filename.split('/')[-1][:-5] + "_Skim.root" 

    job["executable"] = "{}/src/ChargedHiggs/nanoAOD_processing/batch/condor_script.sh".format(os.environ["CMSSW_BASE"])
    job["arguments"] = filename 
    job["universe"]       = "vanilla"

    job["should_transfer_files"] = "YES"
    job["transfer_input_files"]       = ",".join([os.environ["CMSSW_BASE"] + "/src/ChargedHiggs", os.environ["CMSSW_BASE"] + "/src/x509"])

    job["log"]                    = outdir + "/log/job_$(Cluster).log"
    job["output"]                    = outdir + "/log/job_$(Cluster).out"
    job["error"]                    = outdir + "/log/job_$(Cluster).err"
     
    job["when_to_transfer_output"] = "ON_EXIT"
    job["transfer_output_remaps"] = '"' + '{filename} = {outdir}/{filename}'.format(filename=skimfilename, outdir=outdir) + '"'


    with schedd.transaction() as txn:
        job.queue(txn, 1)
        print "Submitting job for file {}".format(filename)

    
def main():
    args = parser()

    filelist = []

    if args.filetxt != None:
        filelist = get_filenames(args.filetxt)

    elif args.filename != None:
        filelist.append(args.filename)

    if args.batch == "local":
        post_processor(args.out_dir, filelist)

    elif args.batch == "condor":
        skimdir = "/nfs/dust/cms/user/{}/Skimdir/nanoSkim_{}".format(os.environ["USER"], "_".join([str(time.localtime()[i]) for i in range(6)]))

        os.system("mkdir -p {}".format(skimdir))        
        os.system("chmod 755 {}".format(os.environ["X509_USER_PROXY"])) 
        os.system("cp -u {} {}/src/".format(os.environ["X509_USER_PROXY"], os.environ["CMSSW_BASE"])) 

        for dirname, filenames in filelist.iteritems():
            outdir = skimdir + "/" + dirname

            os.system("mkdir -p {}/log".format(outdir))

            time.sleep(5)

            for filename in filenames:
                condor_submit(outdir, filename)
       
        
if __name__ == "__main__":
    main()
