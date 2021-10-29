#!/usr/bin/env python3

import argparse
import os
import glob
from pprint import pprint

def parser():
    parser = argparse.ArgumentParser(description = "Script to handle and execute analysis tasks", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--skim-dir", type=str)

    return parser.parse_args()

def main():
    args = parser()

    chDir = os.environ["CHDIR"]
    dCachePath = "/pnfs/desy.de/cms/tier2/store/user/dbrunner/"
    skimDir = os.path.abspath(args.skim_dir)

    for f in glob.glob(skimDir.replace(chDir, dCachePath) +  "/*/*/*/*/merged/*root", recursive=True):
        print(f)

        try:
            os.symlink(f, f.replace(dCachePath, chDir))

        except OSError as e:
            os.remove(f.replace(dCachePath, chDir))
            os.symlink(f, f.replace(dCachePath, chDir))

if __name__ == "__main__":
    main()
