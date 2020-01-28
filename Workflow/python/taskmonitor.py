import curses
import textwrap
from pprint import pprint

class TaskMonitor(object):
    def __init__(self, logWindows=False):
        self.stdscr = curses.initscr()

        #curses.noecho()
        curses.cbreak()
        curses.curs_set(0)

        self.rows = 16
        self.rowMax, self.columns = self.stdscr.getmaxyx()

        ##Define colors
        curses.start_color()
        curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
        curses.init_pair(5, curses.COLOR_BLUE, curses.COLOR_BLACK)
        curses.init_pair(6, curses.COLOR_WHITE, curses.COLOR_BLACK)

        self._colors = {
            "FAILED": curses.color_pair(1),
            "FINISHED": curses.color_pair(2),
            "VALID": curses.color_pair(3),
            "SUBMITTED": curses.color_pair(4),
            "RUNNING": curses.color_pair(5),
        }

        self.stdscr.refresh()

        ##Create Windows
        self._mainWindow = curses.newwin(self.rows, self.columns, 0, 0)
        self._mainWindow.refresh()

        if logWindows:
            self._logWindow = curses.newwin(self.rowMax-16, self.columns, 16, 0)
            self._logWindow.refresh()

            self._printOut = []

        ##Useful lambda functions
        self.centeredMain = lambda string : int(self.columns/2) - int(len(string)/2)

    def __flattenList(self, l):
        niceList = []

        for line in l:
            if len(line) < self.columns-3:
                niceList.append(line)

            else:
                tmpList = textwrap.wrap(line, self.columns-4)
                niceList.extend(tmpList)

        return niceList                
          
    def __del__(self):
        curses.endwin()

    def multistringLine(self, yPos, strings, values, colors=[]):
        if not colors:
            colors = [curses.color_pair(6)]*len(strings)

        totalLen = sum([len(s) for s in strings])

        distance = int((self.columns-totalLen)/(len(strings)+1))
        position = 0

        self._mainWindow.addstr(yPos, 1, " "*(self.columns-3))

        for i, (stat, color, value) in enumerate(zip(strings, colors, values)):
            if i == 0:
                position = distance

            else:
                position = position + distance + len(strings[i-1])            
            
            self._mainWindow.addstr(yPos, position, stat, color)
            self._mainWindow.addstr(yPos, position+len(stat), ": {}".format(value))
            
    def updateMainMonitor(self, layer, time, localStats, condorStats):
        ##Frame Head line
        heading = "TASK LAYER {} (see http://localhost:2000/workflow.html)".format(layer)
        self._mainWindow.addstr(0, self.centeredMain(heading), heading)

        ##Frame
        symbol = "."
        self._mainWindow.addstr(1, 0, symbol*self.columns, curses.A_BOLD)
        self._mainWindow.addstr(self.rows-2, 0, symbol*self.columns, curses.A_BOLD)

        for i in range(self.rows-2):
            self._mainWindow.addstr(i+1, 0, symbol, curses.A_BOLD)
            self._mainWindow.addstr(i+1, self.columns-1, symbol, curses.A_BOLD)

        ##Statistics
        stat = "STATISTICS"
        self._mainWindow.addstr(2, self.centeredMain(stat), stat, curses.A_UNDERLINE)

        names = ["COMPLETED", "TIME PASSED"]
        values = ["{}/{} ({:0.2f} %)".format(
                            localStats["FINISHED"] + condorStats["FINISHED"], 
                            localStats["TOTAL"] + condorStats["TOTAL"], 
                            100*(localStats["FINISHED"] + condorStats["FINISHED"])/(localStats["TOTAL"] + condorStats["TOTAL"])), 
                "{:0.2f} s".format(time)]
        self.multistringLine(4, names, values)

        ##Run information local
        stat = "LOCAL JOBS"
        self._mainWindow.addstr(6, self.centeredMain(stat), stat, curses.A_UNDERLINE)

        status = ["RUNNING", "VALID", "FINISHED", "FAILED"]
        colors = [self._colors[stat] | curses.A_BOLD for stat in status]
        values = [localStats[stat] for stat in status]

        self.multistringLine(8, status, values, colors)      

        ##Run information local
        stat = "CONDOR JOBS"
        self._mainWindow.addstr(10, self.centeredMain(stat), stat, curses.A_UNDERLINE)

        status = ["RUNNING", "SUBMITTED", "VALID", "FINISHED", "FAILED"]
        colors = [self._colors[stat] | curses.A_BOLD for stat in status]
        values = [condorStats[stat] for stat in status]

        self.multistringLine(12, status, values, colors)   
    
        self._mainWindow.refresh()

    def updateLogMonitor(self, logs, errors):
        logs = self.__flattenList(logs)

        ##Frame Head line
        heading = "CONSOLE OUTPUT"
        self._logWindow.addstr(0, self.centeredMain(heading), heading)

        ##Frame
        symbol = "."
        self._logWindow.addstr(1, 0, symbol*self.columns, curses.A_BOLD)
        self._logWindow.addstr(self.rowMax-18, 0, symbol*self.columns, curses.A_BOLD)

        for i in range(self.rowMax-19):
            self._logWindow.addstr(i+1, 0, symbol, curses.A_BOLD)
            self._logWindow.addstr(i+1, self.columns-1, symbol, curses.A_BOLD)

        if len(logs) == 0:
            return None

        ##Handgle printout
        maxLines = self.rowMax-19

        while(True):
            if(len(logs) == 0):
                break

            if(len(self._printOut) != 0):  
                self._printOut.pop(0)

            while(len(self._printOut) != maxLines):
                self._printOut.append(logs.pop(0))

                if(len(logs) == 0):
                    break

            for i in range(maxLines):
                if i+1==len(self._printOut):
                    break

                self._logWindow.addstr(i+2, 2, " "*(self.columns-3)) 
                self._logWindow.addstr(i+2, 2, self._printOut[i])    
                self._logWindow.refresh()
