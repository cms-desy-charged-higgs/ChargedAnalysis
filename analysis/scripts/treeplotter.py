#!/usr/bin/env python

import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import time

import os
import yaml
import argparse

from multiprocessing import Pool, cpu_count

def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--background", nargs="+", default = ["TT+X", "T+X", "QCD", "W+j", "DY+j", "VV+VVV"])
    parser.add_argument("--data", nargs="+", default = [])
    parser.add_argument("--signal", nargs = "+", default = [])   

    parser.add_argument("--process-yaml", type = str, default = "{}/src/ChargedHiggs/analysis/data/process.yaml".format(os.environ["CMSSW_BASE"]))
    parser.add_argument("--skim-dir", type = str)
    parser.add_argument("--plot-dir", type = str, default = "{}/src/Plots".format(os.environ["CMSSW_BASE"]))

    parser.add_argument("--parameters", nargs="+")
    parser.add_argument("--cuts", nargs="+", default=["baseline"])
    parser.add_argument("--weights", type = str, default = "")

    parser.add_argument("--hist-dir", type = str, default = "{}/src/Histograms".format(os.environ["CMSSW_BASE"]))
    parser.add_argument("--www", type=str)

    return parser.parse_args()

def createHistograms(process, filenames, parameters, cuts, outdir):

    outname = ROOT.std.string("{}/{}.root".format(outdir, process))

    timestamp1 = time.time()
    reader = ROOT.TreeReader(ROOT.std.string(process), parameters, cuts)
    reader.EventLoop(filenames)
    reader.Write(outname)
    timestamp2 = time.time()

    print "Created histograms for process: {} ({} s)".format(process, timestamp2 - timestamp1)

def makePlots(histDir, parameters, processes, plotDir):

    plotter = ROOT.Plotter(histDir, parameters)
    plotter.ConfigureHists(processes)
    plotter.Draw(plotDir)
    
def main():
    ##Parser
    args = parser()

    ##Load filenmes of processes
    process_dic = yaml.load(file(args.process_yaml, "r"))

    ##Create plot/hist dir
    if not os.path.exists(args.hist_dir):
        print "Created output path: {}".format(args.hist_dir)
        os.system("mkdir -p {}".format(args.hist_dir))

    if not os.path.exists(args.plot_dir):
        print "Created output path: {}".format(args.plot_dir)
        os.system("mkdir -p {}".format(args.plot_dir))

    if args.www:
        if not os.path.exists(args.www):
            print "Created output path: {}".format(args.www)
            os.system("mkdir -p {}".format(args.www))

    ##Create std::vector for parameters
    parameters = ROOT.std.vector("string")()
    [parameters.push_back(param) for param in args.parameters]

    processes = ROOT.std.vector("string")()
    [processes.push_back(proc) for proc in args.background + args.data + args.signal]

    cuts = ROOT.std.vector("string")()
    [cuts.push_back(cut) for cut in args.cuts]

    histDir = ROOT.std.string(args.hist_dir)
    outDir = ROOT.std.vector("string")()
    outDir.push_back(args.plot_dir)

    if args.www:
        outDir.push_back(args.www)

    ##Create histograms
    for process in processes:
        filenames = ROOT.std.vector("string")()
        
        [filenames.push_back(fname) for fname in ["{skim}{file}/{file}.root".format(skim = args.skim_dir, file = process_file) for process_file in process_dic[process]["filenames"]]]       

        createHistograms(process, filenames, parameters, cuts, histDir)
    
    makePlots(histDir, parameters, processes, outDir)

    if args.www:
        os.system("rsync -arvt --exclude {./,.git*} --delete -e ssh $HOME2/webpage/ $CERN_USER@lxplus.cern.ch:/eos/home-${USER:0:1}/$CERN_USER/www/")
        print "Plots are avaiable on: https://dbrunner.web.cern.ch/dbrunner/"
        

if __name__ == "__main__":
    main()
