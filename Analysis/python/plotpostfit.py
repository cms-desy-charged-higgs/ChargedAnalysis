from task import Task

import os
import ctypes

from ROOT import PlotterPostfit, string, vector

class PostfitPlot(Task):
        super().__init__(config)

    def run(self):
        self["executable"] = "PlotLimit"

        self["arguments"] = [
                "--limit-dir", self["limit-dir"],
                "--mass", self["mass"],
                "--channel", self["channel"]
        ]

        return super()._run()

        plotter = PlotterPostfit(self._stdDir["limit-dir"], self._stdDir["mass"], self._stdDir["channel"])
        plotter.ConfigureHists(vector("string")())
        plotter.Draw(vector("string")(1, self._stdDir["dir"]))

    def output(self):
        self["output"] = self["dir"] + "/postfit_{}.pdf".format(self["mass"])
