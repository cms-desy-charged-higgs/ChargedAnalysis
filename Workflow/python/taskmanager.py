import time
import os
import copy
import subprocess

from taskmonitor import TaskMonitor

import multiprocessing as mp
from collections import deque

class TaskManager(object):
    def __init__(self, tasks, dir = ""):
        self.time = time.time()
        self._tasks = {t["name"]: t for t in tasks}
        
        ##Check for unique name/directory
        if len(tasks) != len(self._tasks.keys()):
            raise RuntimeError("Each task has to have an unique name!")
            
        if len(tasks) != len(set([t["dir"] for t in tasks])):
            raise RuntimeError("Each task has to have an unique directory!")
            
        self.dir = dir if dir else "Tasks/{}".format(time.asctime().replace(" ", "_").replace(":", "_"))
        os.makedirs(self.dir)
        
        self.monitor = TaskMonitor(len(tasks))
        
        self.remove = []
        self.runningTasks = {}
        self.status = {}
        
    def __changeStatus(self, task, status, runType):
        if task["status"] != "None":
            ##Decrement number of old status
            self.status.setdefault(runType, {}).setdefault(task["status"], 0)
            self.status[runType][task["status"]] -= 1
            
        ##Increment number of new status
        self.status.setdefault(runType, {}).setdefault(status, 0)
        self.status[runType][status] += 1
        task["status"] = status
        
        ##Mark task to be removed from self._tasks
        if status == "Finished":
            self.remove.append(task["name"])
        
        ##Break if job failed
        if status == "Failed":
            self.monitor.updateMonitor(time.time() - self.time, self.status)
            raise RuntimeError("Task '{}' failed! See error message: {}/err.txt".format(task["name"], task["dir"]))
        
    def __checkDependency(self, task):
        ##Check for circular dependency with Depth-first search
        if hasattr(task, "Checked"):
            raise RuntimeError("Circular dependency found!")
    
        task.Checked = True
            
        for dep in task["dependencies"]:
            self.__checkDependency(self._tasks.get(dep, {}))
        
    def __getJobs(self):
        pool = mp.Pool(processes=20)
        condorSubmit = "condor_submit tasks.sub -batch-name TaskManager -queue DIR in "
        
        while(self._tasks):
            for name, task in self._tasks.items():
                ##Ignore if running task
                if task["status"] == "Running":
                    continue
            
                ##Check if dependencies all finished and if no circular dependency
                try:          
                    for dep in task["dependencies"]:
                        if not hasattr(task, "Checked"):
                            self.__checkDependency(task)
                    
                        if dep in self._tasks:
                            raise ValueError("")
                            
                except ValueError: continue
                
                ##Local job configuration
                if task["run-mode"] == "Local":
                    task.job = pool.apply_async(copy.deepcopy(task))
                    self.__changeStatus(task, "Running", "Local")
                    self.runningTasks.setdefault("Local", deque()).append(task)
                    
                    if len(self.runningTasks["Local"]) >= 20:
                        yield True
            
                ##Condor job configuration
                if task["run-mode"] == "Condor":
                    self.runningTasks.setdefault("Condor", deque()).append(task)
                    self.__changeStatus(task, "Submitted", "Condor")
            
                    if len(self.runningTasks["Condor"]) > 200:
                        subprocess.run(condorSubmit + " ".join([t["dir"] for t in self.runningTasks["Condor"]]), shell = True, stdout=open(os.devnull, 'wb'))
                        yield True
                
            ##Submit condor if loop over all task finished and n < 200     
            if self.runningTasks.get("Condor", []):
                subprocess.run(condorSubmit + " ".join([t["dir"] for t in self.runningTasks["Condor"]]), shell = True, stdout=open(os.devnull, 'wb'))

            ##Yield back if task loop finished but not all resourced are used
            yield True
                
            ##Remove finished jobs
            while(self.remove):
                self._tasks.pop(self.remove.pop())
                
    def run(self, dryRun = False):
        ##Create directories/executable etc.
        for task in self._tasks.values():
            task.prepare()
            self.__changeStatus(task, "Valid", task["run-mode"])
        
        if dryRun:
            return 0
            
        ##Generator to process list of tasks sequentially
        jobHandler = self.__getJobs()
        next(jobHandler)
            
        ##Tasks are saved in FIFO, if task not finished, its pushed back to FIFO
        while(True):
            if self.runningTasks.get("Local", []):
                task = self.runningTasks["Local"].popleft()

                if task.job.ready():
                    if task.job.get() == 0:
                        self.__changeStatus(task, "Finished", "Local")
                        next(jobHandler, None)
                        
                    else:
                        self.__changeStatus(task, "Failed", "Local")
                     
                else:
                    self.runningTasks["Local"].append(task)
                    
            if self.runningTasks.get("Condor", []):
                task = self.runningTasks["Condor"].popleft()
                
                with open("{}/log.txt".format(task["dir"]), "r") as logFile:
                    condorLog = logFile.read()
                      
                if "Job executing" in condorLog and task["status"] != "Running":
                    self.__changeStatus(task, "Running", "Condor")
             
                if "Normal termination" in condorLog:
                    if "(return value 0)" in condorLog:
                        self.__changeStatus(task, "Finished", "Condor")
                        next(jobHandler, None)
                        continue
                        
                    else:
                        self.__changeStatus(task, "Failed", "Condor")
               
                self.runningTasks["Condor"].append(task)
                
            ##Update monitor
            self.monitor.updateMonitor(time.time() - self.time, self.status)
            
            if not self._tasks and not self.runningTasks.get("Local", []) and not self.runningTasks.get("Condor", []):
                break
