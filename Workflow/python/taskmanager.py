import time
import os
import signal
import copy
import traceback
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
        dirs, self._tasks = [], {}
        
        for t in tasks:
            self._tasks[t["name"]] = t
            dirs.append(t["dir"])
            
        self._taskNames = {}
        self.nTasks, self.preFinished = 0, 0
        
        ##Track jobs
        self.runningTasks, self.status = {}, {}

        ##Create working dir
        self.dir = dir if dir else "Tasks/{}".format(time.asctime().replace(" ", "_").replace(":", "_"))
        os.makedirs(self.dir, exist_ok = True)

        ##Stuff for job starter function
        self.pool = mp.Pool(processes=mp.cpu_count(), initializer = lambda : signal.signal(signal.SIGINT, signal.SIG_IGN))
        self.condorSubmit = "condor_submit {} -batch-name TaskManager".format(condorSub) + (" -append '+RequestRuntime = 86400' " if longCondor else "") + " -queue DIR in "

        if tasks:
            if len(self._tasks) != len(tasks):
                raise RuntimeError("Each task has to have an unique name!")

            if len(self._tasks) != len(dirs):
                raise RuntimeError("Each task has to have an unique directory!")

            ##Write summary to csv file
            self.summary = csv.DictWriter(open("{}/summary.csv".format(self.dir), "w"), delimiter = "\t", fieldnames = ["Status", "Task", "Dir"])
            self.summary.writeheader()

            self.nTasks = len(self._tasks)
        
        elif existingFlow:
            oldFlow = "/tmp/summary.csv"
            subprocess.run("cp {} /tmp/summary.csv".format(existingFlow), shell = True)

            ##Write summary to csv file
            self.summary = csv.DictWriter(open("{}/summary.csv".format(self.dir), "w"), delimiter = "\t", fieldnames = ["Status", "Task", "Dir"])
            self.summary.writeheader()

            with open(oldFlow, "r") as flow:
                summary = csv.reader(flow, delimiter = "\t")
                next(summary, None)

                for (taskStatus, taskName, taskDir) in summary:
                    self.nTasks += 1

                    with open("{}/task.yaml".format(taskDir)) as taskConfig:
                        t = yaml.safe_load(taskConfig)
                        task = Task(t)

                        ##If task already finished from previous execution just continue
                        if taskStatus == "Finished":
                            self.preFinished += 1
                            self.__changeStatus(task, "Finished", task["run-mode"])

                        else:
                            self._tasks[taskName] = task

        else:
            raise RuntimeError("No tasks will be running! You need to provide a list of tasks or a existing workflow by handing the csv summary!")

        ##Monitor displayed
        self.monitor = TaskMonitor(self.nTasks)

        ##Template for workflow.html displaying jobs
        self.html = html
        self.htmlTemplate, self.divTemplate = "", ""

        with open(os.path.expandvars(html), "r") as workflow:
            for line in workflow:
                if "div class='task'" in line:
                    self.divTemplate = line
                    self.htmlTemplate += "{divs}"

                else:
                    self.htmlTemplate += line
        
    def __writeSummary(self):
        ##Write remaining jobs in case of failure
        for name, task in self._tasks.items():
            if task["status"] == "None":
                task.prepare()
            self.summary.writerow({"Task": name, "Dir": task["dir"], "Status": task["status"]})

        for mode in self._taskNames.keys():
            for name, task in self.runningTasks.get(mode, {}).items():
                self.summary.writerow({"Task": name, "Dir": task["dir"], "Status": task["status"]})            

    def __sortTasks(self, dryRun):
        ##Sort tasks by Kahn’s algorithm for topological sorting
        in_degrees, dependencies, queue = {}, {}, [] 
        
        for i, task in enumerate(self._tasks.values()):
            task.idx = i

            if sum([1 for dep in task["dependencies"] if dep in self._tasks]) == 0:
                queue.append(task)

            else:
                in_degrees[i] = sum([1 for dep in task["dependencies"] if dep in self._tasks])

                for dep in task["dependencies"]:
                    if dep in self._tasks:
                        dependencies.setdefault(dep, []).append(task["name"])

        counter = 0

        while queue:
            t = queue.pop(0)
            self._taskNames.setdefault(t["run-mode"], []).append(t["name"])
            self.__changeStatus(t, "Valid", t["run-mode"])

            if not dryRun:
                next(self.__jobStarter, None)
                next(self.__jobFinisher, None)

            else:
                self.summary.writerow({"Task": t["name"], "Dir": t["dir"], "Status": t["status"]})            

            for dep in dependencies.get(t["name"], []):
                idx = self._tasks[dep].idx
            
                in_degrees[idx] -= 1

                if in_degrees[idx] == 0:
                    queue.append(self._tasks[dep])
        
            counter+=1
           
        if counter != self.nTasks - self.preFinished:
            raise RuntimeError("There exists a cyclic dependency in your tasks!")   

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
            
        else:
            task.prepare()
     
        ##Increment number of new status
        self.status.setdefault(runType, {}).setdefault(status, 0)
        self.status[runType][status] += 1
        task["status"] = status

        ##Write workflow.html
      #  self.__writeHTML()

        if status == "Finished":
            self.summary.writerow({"Task": task["name"], "Dir": task["dir"], "Status": status})
        
        ##Break if job failed
        if status == "Failed":
            if task.retries >= 3:
                self.summary.writerow({"Task": task["name"], "Dir": task["dir"], "Status": status})
                self.monitor.updateMonitor(time.time() - self.time, self.status)
                raise RuntimeError("Task '{}' failed! See error message: {}/err.txt".format(task["name"], task["dir"]))

            else:
                task.retries += 1

                self.__changeStatus(task, "Valid", runType)
                self._taskNames[runType].insert(0, task["name"])
                self._tasks[task["name"]] = task
                task.prepare()

    def __jobStarter(self):
        toSubmit = []
        waited = 0

        while(True):
            self.monitor.updateMonitor(time.time() - self.time, self.status)
            yield
                 
            ##No jobs left (everything finished)
            if not self._tasks and len([t for m in self.runningTasks.keys() for t in self.runningTasks[m]]) == 0:
                break
               
            ##No jobs left, but still jobs are running
            elif not self._tasks:
                continue
           
            for mode in self._taskNames.keys():
                ##Pop front task
                if len(self._taskNames[mode]) != 0:
                    name = self._taskNames[mode].pop(0)
                    task = self._tasks.pop(name)

                else:
                    continue

                ##Check if dependencies all finished
                try:          
                    for dep in task["dependencies"]:
                        if dep in self._tasks or dep in [name for m in self.runningTasks.keys() for name in self.runningTasks[m]]:
                            raise ValueError("")
                                
                except ValueError:
                    self._taskNames[mode].insert(min(len(self._taskNames[mode]), 50), name)
                    self._tasks[name] = task
                    
                    continue
                    
                ##Local job configuration
                if task["run-mode"] == "Local":
                    if len(self.runningTasks.get("Local", [])) >= mp.cpu_count():
                        self._taskNames[mode].insert(0, name)
                        self._tasks[name] = task
                        continue

                    task.job = self.pool.apply_async(copy.deepcopy(task.run))

                    self.__changeStatus(task, "Running", "Local")
                    self.runningTasks.setdefault("Local", OrderedDict())[name] = task 
                
                ##Condor job configuration
                if task["run-mode"] == "Condor":
                    if len(self.runningTasks.get("Condor", [])) >= 600:
                        self._taskNames[mode].insert(0, name)
                        self._tasks[name] = task
                        continue

                    waited = 0
                    self.runningTasks.setdefault("Condor", OrderedDict())[name] = task
                    self.__changeStatus(task, "Submitted", "Condor")
                    toSubmit.append(task["dir"])

            ##Submit remaining condors jobs
            if "Condor" in self._taskNames:
                if len(toSubmit) >= 200 or (len(toSubmit) != 0 and waited >= len(self._taskNames["Condor"])):
                    subprocess.run(self.condorSubmit + " ".join(toSubmit), shell = True, stdout=open(os.devnull, 'wb'))
                    toSubmit.clear()

                waited += 1
                    
    def __jobFinisher(self):
        ##Tasks are saved in FIFO, if task not finished, its pushed back to FIFO
        while(True):
            self.monitor.updateMonitor(time.time() - self.time, self.status)
            yield
        
            if self.runningTasks.get("Local", OrderedDict()):
                name, task = self.runningTasks["Local"].popitem(0)

                if task.job.ready():
                    if task.job.get() == 0:
                        self.__changeStatus(task, "Finished", "Local")
                            
                    else:
                        del task.job
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
                    self._taskNames["Condor"].insert(0, name)
                    self._tasks[name] = task
                    task.prepare()
                   
                else:
                    self.runningTasks["Condor"][name] = task
     
    def run(self, dryRun = False):
        try:
            self.__jobStarter = self.__jobStarter()
            self.__jobFinisher = self.__jobFinisher()
     
            self.__sortTasks(dryRun)

            if dryRun:
                return 0
            
            while(self._tasks or len([t for m in self.runningTasks.keys() for t in self.runningTasks[m]]) != 0):
                next(self.__jobStarter, None)
                next(self.__jobFinisher, None)
                self.monitor.updateMonitor(time.time() - self.time, self.status)

        except KeyboardInterrupt:
            self.pool.terminate()
            self.__writeSummary()
            print("\nKeyboardInterrupt! Taskmanager writing summary...")

        except RuntimeError as e:
            self.__writeSummary()
            print("\n{}: {}\nTaskmanager writing summary...".format(type(e).__name__, str(e)))
