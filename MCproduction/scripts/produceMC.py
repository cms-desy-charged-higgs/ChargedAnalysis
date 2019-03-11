#!/usr/bin/env python

import sys
import os
import argparse

sys.path.append("/usr/lib64/python2.6/site-packages/")
import htcondor

def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--MHc", type = int)
    parser.add_argument("--Mh", type = int)
    parser.add_argument("--lhe-dir", type = str)
    parser.add_argument("--fragment", type = str)

    return parser.parse_args()


def condor_submit(MHc, Mh, fragment, lhefile):
    job = htcondor.Submit()
    schedd = htcondor.Schedd()

    name = lhefile.split("/")[-1][:-4]

    outdir = "/nfs/dust/cms/user/{}/Signal/Hc+hTol4b_MHc{}_Mh{}/Samples/".format(os.environ["USER"], MHc, Mh)
    os.system("mkdir -p {}".format(outdir)) 
    os.system("mkdir -p {}/log".format(outdir)) 

    job["executable"] = "{}/src/ChargedHiggs/MCproduction/batch/produceMC.sh".format(os.environ["CMSSW_BASE"])
    job["arguments"] = " ".join([fragment.split("/")[-1], lhefile.split("/")[-1], name])
    job["universe"]       = "vanilla"

    job["should_transfer_files"] = "YES"
    job["transfer_input_files"]       = ",".join([fragment, lhefile, os.environ["CMSSW_BASE"] + "/src/x509", os.environ["HOME"] + "/.dasmaps/"])

    job["log"]                    = "{}/log/job_$(Cluster).log".format(outdir)
    job["output"]                    = "{}/log/job_$(Cluster).out".format(outdir)
    job["error"]                    = "{}/log/job_$(Cluster).err".format(outdir)

    job["when_to_transfer_output"] = "ON_EXIT"
    job["transfer_output_remaps"] = '"' + '{filename}_NANOAOD.root = {outdir}/{filename}_NANOAOD.root'.format(filename=name, outdir=outdir) + '"'

    job["on_exit_hold"] = "(ExitBySignal == True) || (ExitCode != 0)"  
    job["periodic_release"] =  "(NumJobStarts < 100) && ((CurrentTime - EnteredCurrentStatus) > 60)"

    job["+RequestRuntime"]    = "{}".format(60*60*12)

    def submit(schedd, job):
        with schedd.transaction() as txn:
            job.queue(txn)
          
    while(True):
        try: 
            submit(schedd, job)
            print "Submit job for file {}".format(lhefile)
            break    

        except:
            pass

def main():
    args = parser()

    os.system("chmod 755 {}".format(os.environ["X509_USER_PROXY"])) 
    os.system("cp -u {} {}/src/".format(os.environ["X509_USER_PROXY"], os.environ["CMSSW_BASE"])) 

    for fname in os.listdir(args.lhe_dir):
        lhefile = "{}/{}".format(args.lhe_dir, fname)
        condor_submit(args.MHc, args.Mh, args.fragment, lhefile)
    
if __name__ == "__main__":
    main()
