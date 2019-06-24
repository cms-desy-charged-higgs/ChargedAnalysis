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
    miniFiles = ["{}/{}".format(args.signal_dir, mini) for mini in os.listdir(args.signal_dir) if "MINI" in mini]
    
    ##Split hadd process because if too many files it fails
    splittedMiniFiles = [miniFiles[i:i + 100] for i in xrange(0, len(miniFiles), 100)]
    splittedNanoFiles = [nanoFiles[i:i + 100] for i in xrange(0, len(nanoFiles), 100)]

    files = {"MINI": splittedMiniFiles, "NANO": splittedNanoFiles}
    
    for typ in ["MINI"]:
        ##List if intermediate hadded files
        splittedHaddFiles = []
        
        ##Template names of files for hadd command
        splittedTarget = "{}/Hc+hTol4b_MHc{}_Mh{}_{}_{}AOD.root"
        endTarget= "{}/Hc+hTol4b_MHc{}_Mh{}_{}AOD.root".format(args.signal_dir, args.MHc, args.Mh, typ)

        command = {"MINI": "edmCopyPickMerge inputFiles={} outputFile={}", "NANO": "haddnano.py {} {}"}
        delimiter = {"MINI": ",", "NANO": " "}

        for index, splitted in enumerate(files[typ]):
            ##Hadd intermediate targets in list
            target = splittedTarget.format(args.signal_dir, args.MHc, args.Mh, index, typ)
            splittedHaddFiles.append(target)

            if "MINI":
                os.system(command[typ].format(delimiter[typ].join(["file:{}".format(f) for f in splitted]), target))

            else: 
                os.system(command[typ].format(target, delimiter[typ].join(splitted)))

        ##Do the final hadd command
        if "MINI":
            os.system(command[typ].format(delimiter[typ].join(["file:{}".format(f) for f in splittedHaddFiles]), endTarget))

        else:
            os.system(command[typ].format(endTarget, delimiter[typ].join(splittedHaddFiles)))

        for splitted in splittedHaddFiles:
            os.system("command rm {}".format(splitted))

        ##Copy to dCache
        os.system("gfal-mkdir -p 'srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/cms/tier2/store/user/{}/signal/Hc+hTol4b_MHc{}_Mh{}/{}AOD'".format(os.environ["CERN_USER"], args.MHc, args.Mh), typ)
            
        localfile = "{}".format(endTarget)

        dCache = "srm://dcache-se-cms.desy.de:8443//pnfs/desy.de/cms/tier2/store/user/{}/signal/Hc+hTol4b_MHc{}_Mh{}/{}AOD/{}".format(os.environ["CERN_USER"], args.MHc, args.Mh, typ, endTarget.split("/")[-1])

        print "gfal-copy -n 1 -f -r '{}' '{}'".format(localfile, dCache)
        os.system("gfal-copy -n 1 -f -r '{}' '{}'".format(localfile, dCache))

if __name__ == "__main__":
    main()
