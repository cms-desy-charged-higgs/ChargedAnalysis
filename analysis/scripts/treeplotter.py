#!/usr/bin/env python

import ROOT
ROOT.PyConfig.IgnoreCommandLineOptions = True

import os
import yaml
import argparse

from multiprocessing import Pool, cpu_count

def parser():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--processes", nargs="+", default = "all")
    parser.add_argument("--process-yaml", type = str, default = "{}/src/ChargedHiggs/analysis/data/process.yaml".format(os.environ["CMSSW_BASE"]))
    parser.add_argument("--skim-dir", type = str)

    parser.add_argument("--parameters", nargs="+")
    parser.add_argument("--cut", type = str, default = "")
    parser.add_argument("--weights", type = str, default = "")

    parser.add_argument("--hist-dir", type = str, default = "{}/src/Histograms".format(os.environ["CMSSW_BASE"]))

    return parser.parse_args()

def createHistograms(process, filenames, parameters, outdir):

    outname = ROOT.std.string("{}/{}.root".format(outdir, process))

    reader = ROOT.TreeReader(ROOT.std.string(process))
    reader.AddHistogram(parameters)    
    reader.EventLoop(filenames)
    reader.Write(outname)

    print "Created histograms for process: {}".format(process)

def makePlots(histdir, parameters, processes):

    plotter = ROOT.Plotter(histdir)
    plotter.ConfigureHists(parameters, processes)
    plotter.Draw()

    
def main():
    ##Parser
    args = parser()

    ##Load filenmes of processes
    process_dic = yaml.load(file(args.process_yaml, "r"))

    ##Create output dir
    if not os.path.exists(args.hist_dir):
        print "Created output path: {}".format(args.hist_dir)
        os.system("mkdir -p {}".format(args.hist_dir))

    if args.processes == "all":
        args.processes = process_dic.keys()

    ##Create std::vector for parameters
    parameters = ROOT.std.vector("string")()
    [parameters.push_back(param) for param in args.parameters]

    processes = ROOT.std.vector("string")()
    [processes.push_back(proc) for proc in args.processes]

    histDir = ROOT.std.string(args.hist_dir)

    pool = Pool(processes.size())
    results = []

    ##Create histograms
    for process in processes:
        filenames = ROOT.std.vector("string")()
        [filenames.push_back(fname) for fname in ["{skim}{file}/{file}.root".format(skim = args.skim_dir, file = process_file) for process_file in process_dic[process]["filenames"]]]

        results.append(pool.apply_async(createHistograms, args = (process, filenames, parameters, histDir)))
 
    [p.get() for p in results]
    
    makePlots(histDir, parameters, processes)

if __name__ == "__main__":
    main()
