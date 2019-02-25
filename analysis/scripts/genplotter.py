#!/usr/bin/env python

import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os
import argparse

from multiprocessing import Pool, cpu_count

def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--filename", type = str, help = "Filename of the input root file")
    parser.add_argument("--plot-dir", type = str, default = "{}/src/Plots".format(os.environ["CMSSW_BASE"]))
    parser.add_argument("--www", type=str)

    return parser.parse_args()


def makePlotsGen(outDir, filename):

    plotter = ROOT.PlotterGen(filename)
    plotter.ConfigureHists(ROOT.std.vector("string")())
    plotter.FillHists()
    plotter.SetStyle()
    plotter.Draw(outDir)
    
def main():
    ##Parser
    args = parser()

    fileName = ROOT.std.string(args.filename)

    outDir = ROOT.std.vector("string")()
    outDir.push_back(args.plot_dir)

    if args.www:
        if not os.path.exists(args.www):
            print "Created output path: {}".format(args.www)
            os.system("mkdir -p {}".format(args.www))
        outDir.push_back(args.www)

    makePlotsGen(outDir, fileName)

    if args.www:
        os.system("rsync -arvt --exclude {./,.git*} --delete -e ssh $HOME2/webpage/ $CERN_USER@lxplus.cern.ch:/eos/home-${USER:0:1}/$CERN_USER/www/")
        print "Plots are avaiable on: https://dbrunner.web.cern.ch/dbrunner/"

if __name__ == "__main__":
    main()
