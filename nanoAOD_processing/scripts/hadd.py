#!/usr/bin/env python

import os
import argparse
from glob import glob

def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--skim-dir", type = str, required = True)

    return parser.parse_args()


def get_hadd_targets(skimdir):
    filedirs = os.listdir(skimdir)

    haddtargets = {}

    for filedir in filedirs:
        haddtargets[filedir] = glob("{}{}/*.root".format(skimdir, filedir))

    return haddtargets

def main():
    args = parser()

    haddtargets = get_hadd_targets(args.skim_dir)

    for target, rootfiles in haddtargets.iteritems():
        os.system("hadd -j {}/{}/{}.root ".format(args.skim_dir, target, target) + " ".join(rootfiles))

        
if __name__ == "__main__":
    main()
