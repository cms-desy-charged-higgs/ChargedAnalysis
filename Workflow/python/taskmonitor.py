import sys
import os
from time import sleep

class TaskMonitor(object):
    def __init__(self, logWindows=False):
        self.columns, self.rows = os.get_terminal_size()

        self._statusColor = {
            "RUNNING": "blue",
            "VALID": "yellow",
            "SUBMITTED": "magenta",
            "FINISHED": "green",
            "FAILED": "red",
            "TOTAL": "white",
        }

        self.windowOut = []

    def __colorize(self, color, string):
        colors = {
            "green": "\033[92m{}\033[39m",
            "blue": "\033[94m{}\033[39m",
            "red": "\033[91m{}\033[39m",
            "yellow": "\033[93m{}\033[39m",
            "magenta": "\033[95m{}\033[39m",
            "white": "\033[39m{}\033[39m",
            "underline": "\033[4m{}\033[0m",
        }

        return colors[color].format(string)

    def __produceStatLine(self, values):
        totalLen = sum([len(status) + 3 + len(str(value)) for status, value in values.items()])
        distance = int((self.columns-totalLen)/(len(values.keys()) + 1))

        statusLine = ""

        for index, (status, value) in enumerate(values.items()):
            if index == 0:
                statusLine +=  " "*distance

            statusLine += self.__colorize(self._statusColor[status], status) + ": {} ".format(value) + " "*distance

        return statusLine

    def updateMonitor(self, time, localStats, condorStats):
        completedLine = "COMPLETED: {}/{} ({:0.2f} %) \t TIME PASSED: {:0.2f} s".format(
                            localStats["FINISHED"] + condorStats["FINISHED"], 
                            localStats["TOTAL"] + condorStats["TOTAL"], 
                            100*(localStats["FINISHED"]+condorStats["FINISHED"])/(localStats["TOTAL"]+condorStats["TOTAL"]), 
                            time
                        )
        
        if self.windowOut:
            sys.stdout.write(u"\u001b[1000D")
            sys.stdout.write(u"\u001b[{}A".format(len(self.windowOut)))

        self.windowOut = [
            "",
            "TASK MONITOR (see http://localhost:2000/workflow.html)".center(self.columns),
            "."*self.columns, 
            "",
            " "*(int(self.columns/2. - 5)) + self.__colorize("underline", "STATISTICS"),
            "",
            " "*(int(self.columns/4.)) + completedLine,
            "",
            " "*(int(self.columns/2. - 5)) + self.__colorize("underline", "LOCAL JOBS"),
            "",
            self.__produceStatLine(localStats),
            "",
            " "*(int(self.columns/2. - 5)) + self.__colorize("underline", "CONDOR JOBS"),
            "",
            self.__produceStatLine(condorStats),
            "",
            "."*self.columns, 
            "",
        ]

        for out in self.windowOut:
            print(" "*self.columns)
            sys.stdout.write(u"\u001b[1000D")
            sys.stdout.write(u"\u001b[1A")
            print(out)

        sleep(0.1)
