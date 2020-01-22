import numpy as np
from ROOT import TFile, TTree

def ChannelToDir(channel):
    chanToDir = {
            "mu4j": "Muon4J", 
            "e4j": "Ele4J", 
            "mu2j1fj": "Muon2J1FJ", 
            "e2j1fj": "Ele2J1FJ", 
            "mu2fj": "Muon2FJ", 
            "e2fj": "Ele2FJ"
    }

    return chanToDir[channel]

def SplitEventRange(fileName, channel, nEvents):
    rootFile = TFile.Open(fileName)
    rootTree = rootFile.Get(channel)
    entries = rootTree.GetEntries()
    rootFile.Close()

    ##Complicated way of calculating intervals
    intervals = [["0", "0"]]

    if entries != 0:
        nRange = np.arange(0, entries)
        intervals = [[str(i[0]), str(i[-1])] for i in np.array_split(nRange, round(entries/nEvents+0.5))]

    return intervals
