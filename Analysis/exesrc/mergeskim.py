#!/usr/bin/env python3

import argparse
import os
import subprocess
import multiprocessing
import copy
import sys

from ROOT import TFile

from pprint import pprint

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--skim-dir", action = "store", help = "Path with crab dir")
    return parser.parse_args()

def merge(outFile, mergeFiles, optimize = False):
    os.nice(20)

    if not optimize:
        print(subprocess.run(["hadd", "-f", outFile, *mergeFiles], stdout=subprocess.PIPE).stdout.decode('utf-8'))

    else:   
        print(subprocess.run(["hadd", "-f", "-O", outFile, *mergeFiles], stdout=subprocess.PIPE).stdout.decode('utf-8'))

    notToMerge = ["Lumi", "xSec", "pileUp", "pileUpUp", "pileUpDown"]
        
    if(mergeFiles):
        infile = TFile.Open(mergeFiles[0], "READ")
        output = TFile.Open(outFile, "UPDATE")

        for obj in notToMerge:
            try:
                output.Get(obj).GetName()
                output.Delete(obj + ";1")
                infile.Get(obj).Write()

            except ReferenceError:
                pass

        infile.Close()
        output.Close()

    for f in mergeFiles:
        if "tmp" in f:
            subprocess.call(["rm", "-rfv", f])

def main():
    args = parser()
    pool = multiprocessing.Pool(processes = multiprocessing.cpu_count())

    outPaths = ["{}/{}".format(args.skim_dir, path) for path in os.listdir(args.skim_dir)]

    for path in outPaths:
        if not os.path.exists("{}/outputFiles.txt".format(path)):
            continue

        nFilesInParell = 3

        os.makedirs("{}/merged".format(path), exist_ok=True)
        subprocess.call("command rm -fv {}/merged/*".format(path), shell=True)

        tierTwo = "srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/cms/tier2/store/user/dbrunner/{}/merged".format(path[path.find("Skim"):])
        mountedTier = "/pnfs/desy.de/cms/tier2/store/user/dbrunner/{}/merged".format(path[path.find("Skim"):])

        subprocess.call(["gfal-mkdir", "-p", tierTwo])

        wave = 0
        fileBunch = []
        tmpFiles = []

        with open("{}/outputFiles.txt".format(path), "r") as outTxt:
            jobs = []
            lines = list(outTxt.readlines())

            for line in lines:
                fileBunch.append(line.replace("\n", ""))

                if len(fileBunch) >= nFilesInParell:
                    tmpFile = "{}/merged/tmp_{}_{}.root".format(path, wave, len(jobs)) 
                    tmpFiles.append(tmpFile)
    
                    jobs.append(pool.apply_async(merge, args=(tmpFile, fileBunch, True)))

                    fileBunch = []

            [job.get() for job in jobs]

        while(True):
            nFilesInParell = 40

            jobs = []
            oldTmpFiles = copy.deepcopy(tmpFiles)
            tmpFiles = []

            if len(oldTmpFiles) <= nFilesInParell:
                outFile =  "{}/merged/{}.root".format(path, path.split("/")[-1])


                jobs.append(pool.apply_async(merge, args=(outFile, oldTmpFiles)))
                [job.get() for job in jobs]

                subprocess.call(["gfal-copy", "-f", "-p", outFile, tierTwo])

                subprocess.call(["cp", "-fv", "-s",  "{}/{}.root".format(mountedTier, path.split("/")[-1]), outFile])

                break

            else:
                fileBunch = []

                for f in oldTmpFiles:
                    fileBunch.append(f)

                    if len(fileBunch) >= nFilesInParell:
                        tmpFile = "{}/merged/tmp_{}_{}.root".format(path, wave+1, len(jobs))
                        tmpFiles.append(tmpFile)

                        jobs.append(pool.apply_async(merge, args=(tmpFile, fileBunch)))
                        fileBunch = []

                if fileBunch:
                    tmpFile = "{}/merged/tmp_{}_{}.root".format(path, wave+1, len(jobs))
                    tmpFiles.append(tmpFile)

                    jobs.append(pool.apply_async(merge, args=(tmpFile, fileBunch)))
                    
            wave+=1
            [job.get() for job in jobs]
        
if __name__=="__main__":
    main()
