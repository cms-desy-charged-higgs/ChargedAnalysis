import os
import argparse
import sys
import time

sys.path.append("/usr/lib64/python2.6/site-packages/")
import htcondor

def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--MHc", type = int)
    parser.add_argument("--Mh", type = int)
    parser.add_argument("--events-per-run", type = int, default = 1000)
    parser.add_argument("--runs", type = int, default = 2000)

    return parser.parse_args()

def setMadgraph():
    commands = [
                "wget https://launchpad.net/mg5amcnlo/2.0/2.6.x/+download/MG5_aMC_v2.6.4.tar.gz",
                "tar -xf MG5_aMC_v2.6.4.tar.gz",
                "rm MG5_aMC_v2.6.4.tar.gz",
                "wget https://2hdmc.hepforge.org/downloads/2HDMC-1.7.0.tar.gz",
                "tar -xf 2HDMC-1.7.0.tar.gz",
                "mv 2HDMC-1.7.0/MGME/2HDMC/ MG5_aMC_v2_6_4/models/",
                "rm -rf 2HDMC-1.7.0*",
                "mv MG5_aMC_v2_6_4/ {}/src/ChargedHiggs/MCproduction/".format(os.environ["CMSSW_BASE"]),
    ]

    for command in commands:
        os.system(command)

    print "!!!! Change in MG5_aMC_v2_6_4/models/2HDMC/makefile f77 to gfortran !!!!"


def writeCommand(MHc, Mh, nEvents):
    commands = [
                "import model_v4 2HDMC",
                "set fortran_compiler gfortran",
                "define hc = h- h+",
                "define w = w+ w-",
                "define l = l+ l-",
                "define v = vl vl~",
                "generate p p > hc h1, h1 > b b~, (hc > h1 w, h1 > b b~, w > l v)",
                "add process p p > hc h1 j, h1 > b b~, (hc > h1 w, h1 > b b~, w > l v)",
                "add process p p > hc h1 j j, h1 > b b~, (hc > h1 w, h1 > b b~, w > l v)",
                "output Hc+hTol4b_MHc{}_Mh{}".format(MHc, Mh),
                "launch Hc+hTol4b_MHc{}_Mh{}".format(MHc, Mh),
                "Hc+hTol4b_MHc{}_Mh{}.shla".format(MHc, Mh),
                "set nevents {}".format(nEvents),
                "done",       
    ]

    with open(os.environ["CMSSW_BASE"] + "/src/command.txt", "w") as f: 
        for command in commands:
            f.write(command)
            f.write("\n")


def submit(MHc, Mh):
    job = htcondor.Submit()
    schedd = htcondor.Schedd()

    lhefile = "unweighted_events.lhe"
    outdir = "/nfs/dust/cms/user/{}/Signal/Hc+hTol4b_MHc{}_Mh{}/LHE/".format(os.environ["USER"], MHc, Mh)
    outfile = "Hc+hTol4b_MHc{}_Mh{}_{}.lhe".format(MHc, Mh, str(time.time()).replace(".", ""))

    os.system("mkdir -p " + outdir)
    os.system("mkdir -p {}/log".format(outdir)) 

    ##Condor configuration
    job["executable"] = "{}/src/ChargedHiggs/MCproduction/batch/produceLHE.sh".format(os.environ["CMSSW_BASE"])
    job["universe"]       = "vanilla"

    job["should_transfer_files"] = "YES"
    job["transfer_input_files"]       = ",".join([os.environ["CMSSW_BASE"] + "/src/ChargedHiggs/MCproduction/MG5_aMC_v2_6_4", os.environ["CMSSW_BASE"] + "/src/command.txt", os.environ["CMSSW_BASE"] + "/src/ChargedHiggs/MCproduction/SLHA/Hc+hTol4b_MHc{}_Mh{}.shla".format(MHc, Mh)])

    job["log"]                    = "log/job_$(Cluster).log"
    job["output"]                    = "log/job_$(Cluster).out"
    job["error"]                    = "log/job_$(Cluster).err"

    job["when_to_transfer_output"] = "ON_EXIT"
    job["transfer_output_remaps"] = '"' + '{} = {}/{}'.format(lhefile, outdir, outfile) + '"'

    ##Agressively submit your jobs
    def submit(schedd, job):
        with schedd.transaction() as txn:
            job.queue(txn)

    while(True):
        try: 
            submit(schedd, job)
            print "Submit job for LHE file production"
            break    

        except:
            pass


def main():
    args = parser()

    if not os.path.isdir(os.environ["CMSSW_BASE"] + "/src/ChargedHiggs/MCproduction/MG5_aMC_v2_6_4"):
        setMadgraph()
        return 0

    writeCommand(args.MHc, args.Mh, args.events_per_run)

    for i in range(args.runs):
        submit(args.MHc, args.Mh)

if __name__ == "__main__":
    main()
