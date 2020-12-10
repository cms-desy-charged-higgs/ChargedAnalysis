import sys
import os
from time import sleep

class TaskMonitor(object):
    def __init__(self, nTotal):
        self.columns, self.rows = os.get_terminal_size()
        self.nTotal = nTotal
        self.windowOut = []

        self.statusColor = {
            "Running": "blue",
            "Valid": "yellow",
            "Submitted": "magenta",
            "Finished": "green",
            "Failed": "red",
        }

    def __colorize(self, color, status):
        colors = {
            "green": "\033[92m{}\033[39m",
            "blue": "\033[94m{}\033[39m",
            "red": "\033[91m{}\033[39m",
            "yellow": "\033[93m{}\033[39m",
            "magenta": "\033[95m{}\033[39m",
            "white": "\033[39m{}\033[39m",
            "underline": "\033[4m{}\033[0m",
        }

        return colors[color].format(status)

    def __produceLine(self, values):
        totalLen = sum([len(status) + 3 + len(str(value)) for status, value in values.items()])
        distance = int((self.columns-totalLen)/(len(values.keys()) + 1))

        statusLine = ""

        for index, (status, value) in enumerate(values.items()):
            if index == 0:
                statusLine +=  " "*distance

            color = self.statusColor[status] if status in self.statusColor else "white"

            statusLine += self.__colorize(color, status.upper()) + ": {} ".format(value) + " "*distance

        return statusLine

    def updateMonitor(self, time, status):
        sleep(0.1)
    
        nFinished = sum(t.get("Finished", 0) for t in status.values())
    
        statLine = {
            "completed": "{}/{} ({:0.2f} %)".format(nFinished, self.nTotal, 100*nFinished/self.nTotal),
            "time passed": "{:0.2f} s".format(time)
        }
        
        if self.windowOut:
            sys.stdout.write(u"\u001b[1000D")
            sys.stdout.write(u"\u001b[{}A".format(2*len(self.windowOut)))

        self.windowOut = [
            "."*self.columns, 
            " "*(int(self.columns/2. - 5)) + self.__colorize("underline", "STATISTICS"),
            self.__produceLine(statLine),
        ]
        
        for runType, stat in status.items():
            self.windowOut.extend([
                " "*(int(self.columns/2. - len(runType)/2.)) + self.__colorize("underline", runType.upper()),
                self.__produceLine(stat),
            ])
        
        self.windowOut.append("."*self.columns)

        for out in self.windowOut:
            print(" "*self.columns)
            sys.stdout.write(u"\u001b[1000D")
            sys.stdout.write(u"\u001b[1A")
            print(out)
            print("")
