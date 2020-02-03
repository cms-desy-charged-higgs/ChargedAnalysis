import yaml
import copy
import os
import time
import sys
import subprocess
import shutil
import http.server
from multiprocessing import Process, Pool, cpu_count

from taskmonitor import TaskMonitor
from taskwebpage import TaskWebpage

class TaskManager(object):
    def __init__(self, checkOutput=False, noMonitor=False):
        self.checkOutput = checkOutput
        self.noMonitor = noMonitor
        self._graph = []

        ##Make working directory for all output of taskmanager handling
        self.workDir = "{}/Tmp/Workflow/{}/".format(os.environ["CHDIR"], time.asctime().replace(" ", "_").replace(":", "_"))
        os.makedirs(self.workDir, exist_ok=True)

        ##Make HTTP local server for workflow
        http.server.SimpleHTTPRequestHandler.log_message = lambda self, format, *args: None
        os.chdir(self.workDir)

        self.httpd = http.server.HTTPServer(("localhost", 2000), http.server.SimpleHTTPRequestHandler)
        self.server = Process(target=TaskManager.runServer, args=(self.httpd,))
        self.server.daemon = True
        self.server.start()

        ##Logging of output
        self.commands = []
        self.allLogs = []
        self.allErrs = []

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        ##Write log
        with open("{}/log.txt".format(self.workDir), "w") as log:
            for line in self.allLogs:
                log.write(line + "\n")

        ##Write error
        with open("{}/err.txt".format(self.workDir), "w") as err:
            for line in self.allErrs:
                err.write(line + "\n")

        ##Write commands
        with open("{}/commands.txt".format(self.workDir), "w") as command:
            for line in self.commands:
                command.write(line + "\n")

        ##Close http server
        self.httpd.server_close()

    @staticmethod
    def runServer(server):
        server.serve_forever()

    @property
    def tasks(self):
        return self._tasks

    @tasks.setter
    def tasks(self, taskList):
        self._tasks = taskList
                   
    def __createGraph(self):
        allTask = copy.deepcopy(self._tasks)

        ##Find first layer with Task without dependencies
        firstLayer = []

        for task in allTask:
            if(task["dependencies"] == []):
                task["tasklayer"] = 0
                task.createDir()
                task.output()
                firstLayer.append(task)

        for task in firstLayer:
            allTask.remove(task)  

        self._graph.append(firstLayer)
        
        ##Loop over all other Task not in the first task layer
        while(allTask != []):
            nextLayer = []
            for task in allTask:
                isCorrectLayer = True

                ##Check if task has depencies in next layer or in other layer
                for dep in task["dependencies"]:
                    if dep not in [t["name"] for taskLayer in self._graph for t in taskLayer]:
                        isCorrectLayer = False
                
                ##Fill task in layer    
                if isCorrectLayer:
                    task["tasklayer"] = len(self._graph)
                    nextLayer.append(task)
                 
            self._graph.append(nextLayer)

            for task in nextLayer:
                allTask.remove(task)   

    def __printRunStatus(self, layer, time, logs, errors):
        ##Helper function:
        nStatus = lambda l, status: len([1 for task in l if task["status"] == status])

        localStats = {
            "RUNNING": nStatus(self.localTask, "RUNNING"),
            "VALID": nStatus(self.localTask, "VALID"),
            "FINISHED": nStatus(self.localTask, "FINISHED"),
            "FAILED": nStatus(self.localTask, "FAILED"),
            "TOTAL": len(self.localTask),
        }

        condorStats = {
            "RUNNING": nStatus(self.condorTask, "RUNNING"),
            "VALID": nStatus(self.condorTask, "VALID"),
            "SUBMITTED": nStatus(self.condorTask, "SUBMITTED"),
            "FINISHED": nStatus(self.condorTask, "FINISHED"),
            "FAILED": nStatus(self.condorTask, "FAILED"),
            "TOTAL": len(self.condorTask),
        }

        if not self.noMonitor:
            self.monitor.updateMainMonitor(layer, time, localStats, condorStats)
            self.monitor.updateLogMonitor(logs, errors)

    def __submitCondor(self, dirs):
        ##List which will be written into submit file
        condorSub = [
            "universe = vanilla",
            "executable = $(DIR)/run.sh",
            "transfer_executable = True",
            "getenv = True",
            'requirements = (OpSysAndVer =?= "CentOS7")',
            "log = $(DIR)/log.txt",
            "error = $(DIR)/err.txt",
            "output = $(DIR)/out.txt",
        ]

        ##Write submit file
        with open("{}/jobs.sub".format(self.workDir), "w") as condFile:
            for line in condorSub:
                condFile.write(line + "\n")

            condFile.write("queue DIR in ({})".format(" ".join(dirs)))

        result = subprocess.run(["condor_submit", "{}/jobs.sub".format(self.workDir)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
    def runTasks(self):
        ##Helper function:
        nStatus = lambda l, status: len([1 for task in l if task["status"] == status])

        ##Create dependency graph
        self.__createGraph()
                
        ##Task monitor instance
        if not self.noMonitor:
            self.monitor = TaskMonitor(True)
        self.webpage = TaskWebpage()

        for layer, taskLayer in enumerate(self._graph):
            ##Indicate in log task layer
            self.allLogs.append("#"*40 + " TASK LAYER {} ".format(layer) + "#"*40)
            self.allErrs.append("#"*40 + " TASK LAYER {} ".format(layer) + "#"*40)

            self.localTask = []
            self.condorTask = []
            startTime = time.time()

            for task in taskLayer:
                if task["run-mode"] == "Local":
                    task.getDependentFiles(self._graph)
                    task.createDir()
                    task.output()
                    task.run()

                    ##Skip task if already finished and you dont want to rerun
                    if self.checkOutput:
                        if task.checkOutput():
                            task["status"] = "FINISHED"

                    self.localTask.append(task)
                  
                if task["run-mode"] == "Condor":
                    task.getDependentFiles(self._graph)
                    task.createDir()
                    task.output()
                    task.run()

                    ##Skip task if already finished and you dont want to rerun
                    if self.checkOutput:
                        if task.checkOutput():
                            task["status"] = "FINISHED"

                    self.condorTask.append(task)

                ##Write execute commands down
                self.commands.append(task["name"])
                self.commands.append(" ".join([task["executable"], *[str(s) for s in task["arguments"]]]) + "\n")

            ##Submit condors jobs
            dirs = []

            for index, task in enumerate(self.condorTask):
                if(task["status"] != "FINISHED"):
                    task()
                    dirs.append(task["condor-dir"])
                    task["status"] = "SUBMITTED"

            self.__submitCondor(dirs)

            ##Run all local task
            nCores = cpu_count()
            pool = Pool(processes=nCores)
            localRuns = {task: pool.apply_async(task) for task in self.localTask if task["status"] != "FINISHED"}

            while(True):
                logs = []
                errors = []
                self.webpage.createWebpage(self._graph, self.workDir) 

                ##Track local jobs
                for task in self.localTask:
                    if(task["status"] == "FINISHED"):
                        continue

                    if localRuns[task].ready():
                        if localRuns[task].successful():
                            task["status"] = "FINISHED"
                            logs.extend(localRuns[task].get())
                            localRuns.pop(task)

                        else:
                            errors.extend(localRuns[task].get())
                            task["status"] = "FAILED"

                            self.allLogs.extend(logs)
                            self.allErrs.extend(errors)
                            self.__printRunStatus(layer, time.time() - startTime, logs, errors)
                            self.webpage.createWebpage(self._graph, self.workDir)

                            raise RuntimeError("Local job failed!")
                            
                    else:
                        nRuns = nStatus(self.localTask, "RUNNING")
                        nFinished = nStatus(self.localTask, "FINISHED")
                        nFailed = nStatus(self.localTask, "FAILED")
       
                        if nRuns < nCores and nRuns < len(self.localTask) - nFinished - nFailed:
                            task["status"] = "RUNNING"
                           
                ##Track condor jobs
                for task in self.condorTask:
                    #If job not submitted yet, there is no condor.log file
                    if task["status"] in ["VALID", "FINISHED", "FAILED"]:
                        continue

                    with open("{}/log.txt".format(task["condor-dir"])) as logFile:
                        if True in ["Job executing" in line for line in logFile.readlines()]:
                            task["status"] = "RUNNING"

                        logFile.seek(0)

                        if True in ["Normal termination" in line for line in logFile.readlines()]:
                            logFile.seek(0)
                
                            if True in ["(return value 0)" in line for line in logFile.readlines()]:
                                task["status"] = "FINISHED"

                                with open("{}/out.txt".format(task["condor-dir"])) as outFile:
                                    for line in outFile:
                                        logs.append(line)

                            else:
                                task["status"] = "FAILED"

                                with open("{}/err.txt".format(task["condor-dir"])) as errFile:
                                    for line in errFile:
                                        errors.append(line)

                        logFile.seek(0)

                        if True in ["SYSTEM_PERIODIC_REMOVE" in line for line in logFile.readlines()]:
                            task["status"] = "VALID"
                            task["run-mode"] = "Local"
                            self.localTask.append(task)
                            localRuns[task] = pool.apply_async(task)
                            self.condorTask.remove(task)

                nSubmitted = nStatus(self.condorTask, "SUBMITTED") + nStatus(self.condorTask, "RUNNING")

                ##Submit more condors jobs up to 200 if jobs left
                dirs = []

                for task in self.condorTask:
                    if task["status"] == "VALID":
                        if nSubmitted < 200:
                            task.run()
                            dirs.append(task["condor-dir"])
                            task["status"] = "SUBMITTED"
                            nSubmitted+=1

                self.__submitCondor(dirs)

                ##Print status
                self.allLogs.extend(logs)
                self.allErrs.extend(errors)
                self.__printRunStatus(layer, time.time() - startTime, logs, errors)

                ##Draw graph
                self.webpage.createWebpage(self._graph, self.workDir)

                if nStatus(self.localTask, "FINISHED") == len(self.localTask) and nStatus(self.condorTask, "FINISHED") == len(self.condorTask):
                    break
