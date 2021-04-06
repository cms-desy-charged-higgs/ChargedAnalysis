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
    def __init__(self, tasks = [], existingFlow = "", dir = "", condorSub = "$CHDIR/ChargedAnalysis/Workflow/templates/tasks.sub", html = "$CHDIR/ChargedAnalysis/Workflow/templates/workflow.html", longCondor = False):
        self.time = time.time()

        ##Read in tasks or create from given csv summary file of other workflow
        self._tasks = {t["name"] : t for t in tasks}

        if tasks:
            ##Check for unique name/directory
            names = list(self._tasks.keys())
            nonUnique = list(set([n for n in names if names.count(n) > 1]))

            if nonUnique:
                raise RuntimeError("Each task has to have an unique name! Non unique names: \n{}".format("\n".join(nonUnique)))

            dirs = [t["dir"] for t in tasks]  
            nonUnique = list(set([d for d in dirs if dirs.count(d) > 1]))

            if nonUnique:
                raise RuntimeError("Each task has to have an unique directory! Non unique names: \n{}".format("\n".join(nonUnique)))
        
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
        self.longCondor = longCondor
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
        self.monitor = TaskMonitor(len(self._tasks))
        
        ##Track jobs
        self.runningTasks = {}
        self.status = {}

        ##Write summary to csv file
        self.summary = csv.DictWriter(open("{}/summary.csv".format(self.dir), "w"), delimiter = "\t", fieldnames = ["Status", "Task", "Dir"])
        self.summary.writeheader()

    def __del__(self):
        ##Write remaining jobs in case of failure
        for t in self._tasks.values():
            self.summary.writerow({"Task": t["name"], "Dir": t["dir"], "Status": t["status"]})
            
    def __sortTasks(self):
        ##Sort tasks by Kahn’s algorithm for topological sorting
        in_degrees = [len(task["dependencies"]) for task in self._tasks.values()]
        queue = [task for task in self._tasks.values() if len(task["dependencies"]) == 0]

        sortedTasks = {}
        counter=0

        while queue:
            t = queue.pop(0)
            sortedTasks.setdefault(t["run-mode"], []).append(t["name"])

            for i, task in enumerate(self._tasks.values()):
                if t["name"] in task["dependencies"]:
                    in_degrees[i] -= 1

                    if in_degrees[i] == 0:
                        queue.append(task)
        
            counter+=1
           
        if counter != len(self._tasks):
            raise RuntimeError("There exists a cyclic dependency in your tasks!")
    
        else:
            self._taskNames = sortedTasks    

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
     
        ##Increment number of new status
        self.status.setdefault(runType, {}).setdefault(status, 0)
        self.status[runType][status] += 1
        task["status"] = status

        ##Write workflow.html
      #  self.__writeHTML()
        
        ##Break if job failed
        if status == "Failed":
            self.monitor.updateMonitor(time.time() - self.time, self.status)
            raise RuntimeError("Task '{}' failed! See error message: {}/err.txt".format(task["name"], task["dir"]))

    def __getJobs(self):
        pool = mp.Pool(processes=20)
        condorSubmit = "condor_submit {} -batch-name TaskManager".format(self.condorSub) + (" -append '+RequestRuntime = 86400' " if self.longCondor else "") + " -queue DIR in "
        toSubmit = []  
        
        runModes = list(self._taskNames.keys())
        lookForward = {mode : 0 for mode in runModes}  

        while(True):
            yield
                 
            ##No jobs left (everything finished)
            if not self._tasks and len([t for m in self.runningTasks.keys() for t in self.runningTasks[m]]) == 0:
                break
               
            ##No jobs left, but still jobs are running
            elif not self._tasks:
                yield
                continue
           
            for mode in runModes:
                if(lookForward[mode] >= len(self._taskNames[mode])):
                    lookForward[mode] = 0

                ##Pop front task
                if len(self._taskNames[mode]) != 0:
                    name = self._taskNames[mode].pop(lookForward[mode])
                    task = self._tasks.pop(name)

                else:
                    continue

                ##Check if dependencies all finished
                try:          
                    for dep in task["dependencies"]:
                        if dep in self._tasks or dep in [name for m in self.runningTasks.keys() for name in self.runningTasks[m]]:
                            raise ValueError("")
                            
                    lookForward[mode] = 0
                                
                except ValueError:
                    self._taskNames[mode].insert(lookForward[mode], name)
                    self._tasks[name] = task
           
                    lookForward[mode] += 1
                    
                    continue
                    
                ##Local job configuration
                if task["run-mode"] == "Local":
                    if len(self.runningTasks.get("Local", [])) >= 20:
                        self._taskNames[mode].insert(lookForward[mode], name)
                        self._tasks[name] = task
                        continue

                    task.job = pool.apply_async(copy.deepcopy(task.run))

                    self.__changeStatus(task, "Running", "Local")
                    self.runningTasks.setdefault("Local", OrderedDict())[name] = task 
                
                ##Condor job configuration
                if task["run-mode"] == "Condor":
                    if len(self.runningTasks.get("Condor", [])) >= 600:
                        self._taskNames[mode].insert(lookForward[mode], name)
                        self._tasks[name] = task
                        continue

                    self.runningTasks.setdefault("Condor", OrderedDict())[name] = task
                    self.__changeStatus(task, "Submitted", "Condor")
                    toSubmit.append(task["dir"])

            ##Submit remaining condors jobs
            if "Condor" in lookForward and (len(toSubmit) >= 200 or (lookForward["Condor"] >= len(self._taskNames["Condor"]) and len(toSubmit) != 0)):
                subprocess.run(condorSubmit + " ".join(toSubmit), shell = True, stdout=open(os.devnull, 'wb'))
                toSubmit.clear()

    def run(self, dryRun = False):
        ##Sort tasks
        self.__sortTasks()
    
        ##Create directories/executable etc.
        for task in self._tasks.values():
            self.__changeStatus(task, "Valid", task["run-mode"])
            task.prepare()
                
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
                        
                    else:
                        self.__changeStatus(task, "Failed", "Condor")

                elif "SYSTEM_PERIODIC_REMOVE" in condorLog:
                    self.__changeStatus(task, "Valid", "Condor")
                    self._taskNames["Condor"].append(name)
                    self._tasks[name] = task
                    task.prepare()
               
                else:
                    self.runningTasks["Condor"][name] = task

            ##Update monitor
            self.monitor.updateMonitor(time.time() - self.time, self.status)

            next(jobHandler, None)
               
            if not self._tasks and len([t for m in self.runningTasks.keys() for t in self.runningTasks[m]]) == 0:
                break
