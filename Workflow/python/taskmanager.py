import time
import os
import copy
import subprocess
import csv
import json
import yaml
import multiprocessing as mp
from collections import OrderedDict

from taskmonitor import TaskMonitor
from task import Task

class TaskManager(object):
    def __init__(self, tasks = [], existingFlow = "", dir = "", condorSub = "$CHDIR/ChargedAnalysis/Workflow/templates/tasks.sub", html = "$CHDIR/ChargedAnalysis/Workflow/templates/workflow.html"):
        self.time = time.time()

        ##Read in tasks or create from given csv summary file of other workflow
        self._tasks = {}

        if tasks:
            self._tasks = {t["name"]: t for t in tasks}

            ##Check for unique name/directory
            if len(tasks) != len(self._tasks.keys()):
                raise RuntimeError("Each task has to have an unique name!")

            if len(tasks) != len(set([t["dir"] for t in tasks])):
                raise RuntimeError("Each task has to have an unique directory!")
        
        elif existingFlow:
            with open(existingFlow, "r") as flow:
                summary = csv.reader(flow, delimiter = "\t")
                next(summary, None)

                for (taskStatus, taskName, taskDir) in summary:
                    with open("{}/task.yaml".format(taskDir)) as task:
                        t = yaml.safe_load(task)

                        if taskStatus == "Finished":
                            continue

                        self._tasks[taskName] = Task(t, "--")

        else:
            raise RuntimeError("No tasks will be running! You need to provide a list of tasks or a existing workflow by handing the csv summary!")
                        
        ##Create working dir
        self.dir = dir if dir else "Tasks/{}".format(time.asctime().replace(" ", "_").replace(":", "_"))
        os.makedirs(self.dir, exist_ok = True)

        self.condorSub = condorSub
        self.html = html

        ##Template for workflow.html displaying jobs
        self.htmlTemplate, self.divTemplate = "", ""

        with open(os.path.expandvars(html), "r") as workflow:
            for line in workflow:
                if "div class='task'" in line:
                    self.divTemplate = line
                    self.htmlTemplate += "{divs}"

                else:
                    self.htmlTemplate += line
        
        ##Monitor displayed
        self.monitor = TaskMonitor(len(self._tasks.keys()))
        
        self.remove = []
        self.runningTasks = {}
        self.status = {}

        ##Write summary to csv file
        self.summary = csv.DictWriter(open("{}/summary.csv".format(self.dir), "w"), delimiter = "\t", fieldnames = ["Status", "Task", "Dir"])
        self.summary.writeheader()

    def __del__(self):
        ##Write remaining jobs in case of failure
        for t in self._tasks.values():
            self.summary.writerow({"Task": t["name"], "Dir": t["dir"], "Status": t["status"]})

    def __writeHTML(self):
        finished = ""
        running = ""

        ##Loop over all jobs and create div element with template
        for name, t in self._tasks.items():
            div = self.divTemplate.format(data = json.dumps(t, separators=(',', ':')), id = name, dependent = ",".join(t["dependencies"]))

            if t["status"] != "Finished":
                running += div

            else:
                finished += div

        ##Create workflow html
        self.htmlTemplate = self.htmlTemplate.replace("{divs}", finished + "{divs}")

        with open("{}/workflow.html".format(self.dir), "w") as workflow:
            workflow.write(self.htmlTemplate.replace("{divs}", running))

    def __changeStatus(self, task, status, runType):
        if task["status"] != "None":
            ##Decrement number of old status
            self.status.setdefault(runType, {}).setdefault(task["status"], 0)
            self.status[runType][task["status"]] -= 1
            
        ##Mark task to be removed from self._tasks
        if status == "Finished":
            self.remove.append(task["name"])

        ##Increment number of new status
        self.status.setdefault(runType, {}).setdefault(status, 0)
        self.status[runType][status] += 1
        task["status"] = status

        ##Write workflow.html
       # self.__writeHTML()
        
        ##Break if job failed
        if status == "Failed":
            self.monitor.updateMonitor(time.time() - self.time, self.status)
            raise RuntimeError("Task '{}' failed! See error message: {}/err.txt".format(task["name"], task["dir"]))
        
    def __getJobs(self):
        pool = mp.Pool(processes=20)
        condorSubmit = "condor_submit {} -batch-name TaskManager -queue DIR in ".format(self.condorSub)
        
        while(self._tasks):
            toSubmit = []

            ##Remove finished jobs
            while(self.remove):
                t = self._tasks.pop(self.remove.pop())
                self.summary.writerow({"Task": t["name"], "Dir": t["dir"], "Status": t["status"]})

            for name, task in self._tasks.items():
                if task["status"] != "Valid":
                    continue

                ##Check if dependencies all finished
                try:          
                    for dep in task["dependencies"]:
                        if dep in self._tasks:
                            raise ValueError("")
                            
                except ValueError:
                    continue
                
                ##Local job configuration
                if task["run-mode"] == "Local":
                    if len(self.runningTasks.get("Local", [])) >= 20:
                        continue

                    task.job = pool.apply_async(copy.deepcopy(task.run))

                    self.__changeStatus(task, "Running", "Local")
                    self.runningTasks.setdefault("Local", OrderedDict())[name] = task 
            
                ##Condor job configuration
                if task["run-mode"] == "Condor":
                    if len(self.runningTasks.get("Condor", [])) >= 500:
                        continue

                    self.runningTasks.setdefault("Condor", OrderedDict())[name] = task
                    self.__changeStatus(task, "Submitted", "Condor")
                    toSubmit.append(task["dir"])
                
            if toSubmit:
                subprocess.run(condorSubmit + " ".join(toSubmit), shell = True, stdout=open(os.devnull, 'wb'))

            yield True

    def run(self, dryRun = False):
        ##Create directories/executable etc.
        for task in self._tasks.values():
            task.prepare()
            self.__changeStatus(task, "Valid", task["run-mode"])
        
        if dryRun:
            return 0

        ##Update monitor
        self.monitor.updateMonitor(time.time() - self.time, self.status)
            
        ##Generator to process list of tasks sequentially
        jobHandler = self.__getJobs()
        next(jobHandler)
            
        ##Tasks are saved in FIFO, if task not finished, its pushed back to FIFO
        while(True):
            if self.runningTasks.get("Local", OrderedDict()):
                name, task = self.runningTasks["Local"].popitem(0)

                if task.job.ready():
                    if task.job.get() == 0:
                        self.__changeStatus(task, "Finished", "Local")
                        next(jobHandler, None)
                        
                    else:
                        self.__changeStatus(task, "Failed", "Local")
                     
                else:
                    self.runningTasks["Local"][name] = task
                    
            if self.runningTasks.get("Condor", []):
                name, task = self.runningTasks["Condor"].popitem(0)
                
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
               
                self.runningTasks["Condor"][name] = task
               
            ##Update monitor
            self.monitor.updateMonitor(time.time() - self.time, self.status)

            if not self._tasks:
                break
