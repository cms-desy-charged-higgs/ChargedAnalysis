#!/usr/bin/env python

import sys
import os
import argparse

from pprint import pprint

def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--MHc", type = str)
    parser.add_argument("--Mh", type = str)
    parser.add_argument("--signal-dir", type = str)

    return parser.parse_args()

def main():
    ##Read arguments
    args = parser()

    ##Get all files of NANO AOD
    nanoFiles = ["{}/{}".format(args.signal_dir, nano) for nano in os.listdir(args.signal_dir) if "NANO" in nano]
    
    ##Split hadd process because if too many files it fails
    splittedNanoFiles = [nanoFiles[i:i + 100] for i in xrange(0, len(nanoFiles), 100)]
    
    ##List if intermediate hadded files
    splittedHaddFiles = []
    
    ##Template names of files for hadd command
    splittedTarget = "{}/Hc+hTol4b_MHc{}_Mh{}_{}_NANOAOD.root"
    endTarget= "{}/Hc+hTol4b_MHc{}_Mh{}_NANOAOD.root".format(args.signal_dir, args.MHc, args.Mh)

    for index, splitted in enumerate(splittedNanoFiles):
        ##Hadd intermediate targets in list
        target = splittedTarget.format(args.signal_dir, args.MHc, args.Mh, index)
        splittedHaddFiles.append(target)

        os.system("haddnano.py {} {}".format(target, " ".join(splitted)))

    ##Do the final hadd command
    os.system("haddnano.py {} {}".format(endTarget, " ".join(splittedHaddFiles)))

    for splitted in splittedHaddFiles:
        os.system("command rm {}".format(splitted))

    ##Copy to dCache
    os.system("gfal-mkdir -p 'srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/cms/tier2/store/user/{}/signalMC/Hc+hTol4b_MHc{}_Mh{}/NANOAOD'".format(os.environ["CERN_USER"], args.MHc, args.Mh))
        
    localfile = "{}".format(endTarget)

    dCache = "srm://dcache-se-cms.desy.de:8443//pnfs/desy.de/cms/tier2/store/user/{}/signalMC/Hc+hTol4b_MHc{}_Mh{}/NANOAOD/{}".format(os.environ["CERN_USER"], args.MHc, args.Mh, endTarget.split("/")[-1])

    print "gfal-copy -n 1 -f -r '{}' '{}'".format(localfile, dCache)
    os.system("gfal-copy -n 1 -f -r '{}' '{}'".format(localfile, dCache))

if __name__ == "__main__":
    main()
