import yaml
import copy
import os
import time
import sys
import subprocess
import shutil

from multiprocessing import Pool, cpu_count

from taskmonitor import TaskMonitor
from taskwebpage import TaskWebpage

class TaskManager(object):
    def __init__(self, checkOutput=False):
        self.checkOutput = checkOutput
        self._graph = []

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
                    task.createDir()
                    task.output()
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

        self.monitor.updateMainMonitor(layer, time, localStats, condorStats)
        self.monitor.updateLogMonitor(logs, errors)

    def __submitCondor(self, dirs):
        ##Dic which will be written into submit file
        condorDic = {
            "universe": "vanilla",
            "executable": "$(DIR)/run.sh",
            "transfer_executable": "True",
            "getenv": "True",
            "requirements": 'OpSysAndVer =?= "CentOS7"'
            "log": "$(DIR)/log.txt",
            "error": "$(DIR)/err.txt",
            "output":"$(DIR)/out.txt",
        }

        ##Write submit file
        with open("{}/jobs.sub".format(os.environ["CHDIR"]), "w") as condFile:
            for key, value in condorDic.items():
                condFile.write("{} = {}\n".format(key, value))

            condFile.write("queue DIR in ({})".format(" ".join(dirs)))

        result = subprocess.run(["condor_submit", "{}/jobs.sub".format(os.environ["CHDIR"])], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
    def runTasks(self):
        ##Helper function:
        nStatus = lambda l, status: len([1 for task in l if task["status"] == status])

        ##Create dependency graph
        self.__createGraph()   

        ##Task monitor instance
        self.monitor = TaskMonitor(True)
        self.webpage = TaskWebpage()

        for layer, taskLayer in enumerate(self._graph):
            self.localTask = []
            self.condorTask = []
            startTime = time.time()

            for task in taskLayer:
                ##Skip task if already finished and you dont want to rerun
                if self.checkOutput:
                    if task.checkOutput():
                        task["status"] = "FINISHED"

                if task["run-mode"] == "Local":
                    task.getDependentFiles(self._graph)
                    self.localTask.append(task)
                  
                if task["run-mode"] == "Condor":
                    task.getDependentFiles(self._graph)
                    self.condorTask.append(task)

            ##Submit condors jobs (Only 500 hundred at one time)
            dirs = []

            for index, task in enumerate(self.condorTask):
                if(task["status"] != "FINISHED"):
                    task.run()
                    dirs.append(task["condor-dir"])
                    task["status"] = "SUBMITTED"

                if index > 499:
                    break

            self.__submitCondor(dirs)

            ##Run all local task
            nCores = cpu_count()
            pool = Pool(processes=nCores)
            localRuns = {task: pool.apply_async(task) for task in self.localTask if task["status"] != "FINISHED"}

            while(True):
                logs = []
                errors = []
                self.webpage.createWebpage(self._graph) 

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
                            errors.extend(job.get())
                            task["status"] = "FAILED"
                            self.__printRunStatus(layer, time.time() - startTime, logs, errors)
                            self.webpage.createWebpage(self._graph)

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

                    with open("{}/condor.log".format(task["condor-dir"])) as logFile:
                        if True in ["Job executing" in line for line in logFile.readlines()]:
                            task["status"] = "RUNNING"

                        logFile.seek(0)

                        if True in ["Normal termination" in line for line in logFile.readlines()]:
                            logFile.seek(0)
                
                            if True in ["(return value 0)" in line for line in logFile.readlines()]:
                                task["status"] = "FINISHED"

                                with open("{}/condor.out".format(task["condor-dir"])) as outFile:
                                    for line in outFile:
                                        logs.append(line)

                            else:
                                task["status"] = "FAILED"

                                with open("{}/condor.err".format(task["condor-dir"])) as errFile:
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
                self.__printRunStatus(layer, time.time() - startTime, logs, errors)

                ##Draw graph
                self.webpage.createWebpage(self._graph)

                if nStatus(self.localTask, "FINISHED") == len(self.localTask) and nStatus(self.condorTask, "FINISHED") == len(self.condorTask):
                    break
