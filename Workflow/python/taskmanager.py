import yaml
import os
import time
import sys
import subprocess
import http.server
import pprint
from multiprocessing import Process, Pool, cpu_count

from taskmonitor import TaskMonitor
from taskwebpage import TaskWebpage

class TaskManager(object):
    def __init__(self, checkOutput=False, noMonitor=False):
        self.checkOutput = checkOutput
        self.noMonitor = noMonitor

        ##Make working directory for all output of taskmanager handling
        self.workDir = "{}/Workflow/{}/".format(os.environ["CHDIR"], time.asctime().replace(" ", "_").replace(":", "_"))
        os.makedirs(self.workDir, exist_ok=True)

        ##Make HTTP local server for workflow
        http.server.SimpleHTTPRequestHandler.log_message = lambda self, format, *args: None
        os.chdir(self.workDir)

        self.httpd = http.server.HTTPServer(("localhost", 2000), http.server.SimpleHTTPRequestHandler)
        self.server = Process(target=TaskManager.runServer, args=(self.httpd,))
        self.server.daemon = True
        self.server.start()

        ##Task monitor instance
        if not self.noMonitor:
            self.monitor = TaskMonitor(True)
        self.webpage = TaskWebpage()

        ##Start time
        self.startTime = time.time()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.__printRunStatus(time.time() - self.startTime)   
        self.webpage.createWebpage(self._tasks, self.workDir) 

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
                   
    def __sortTasks(self):
        ##Sort tasks by Kahn’s algorithm for topological sorting
        in_degrees = [len(task["dependencies"]) for task in self._tasks]
        queue = [task for task in self._tasks if len(task["dependencies"]) == 0]

        sortedTasks = []
        counter=0

        while queue:
            t = queue.pop(0)
            sortedTasks.append(t)
        
            for i, task in enumerate(self._tasks):
                if task["name"] in t["dependencies"]:
                    t.depth = task.depth + 1
                    t.dependencies.append(task)

                in_degrees[i] -= 1
                    
                if in_degrees[i] == 0:
                    queue.append(task)

            counter+=1
                
        if counter != len(self._tasks):
            raise RuntimeError("There exists a cyclic dependency in your tasks!")
    
        else:
            self._tasks = sortedTasks
    
    def __printRunStatus(self, time):
        ##Helper function:
        nStatus = lambda tasks, stat: len([1 for task in tasks if task["status"] == stat] if stat != "TOTAL" else tasks)

        localStatus = ["RUNNING", "VALID", "FINISHED", "FAILED", "TOTAL"]
        condorStatus = ["RUNNING", "VALID", "SUBMITTED", "FINISHED", "FAILED", "TOTAL"]

        if not self.noMonitor:
            self.monitor.updateMonitor(
                    time, 
                    {stat: nStatus([t for t in self._tasks if t["run-mode"] == "Local"], stat) for stat in localStatus}, 
                    {stat: nStatus([t for t in self._tasks if t["run-mode"] == "Condor"], stat) for stat in condorStatus}
            )

    def __submitCondor(self, tasks):
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

        for task in tasks:
            task()
            task["status"] = "SUBMITTED"

        ##Write submit file
        with open("{}/jobs.sub".format(self.workDir), "w") as condFile:
            for line in condorSub:
                condFile.write(line + "\n")

            condFile.write("queue DIR in ({})".format(" ".join([task["dir"] for task in tasks])))

        subprocess.run(["condor_submit", "{}/jobs.sub".format(self.workDir)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
    def run(self):
        ##Check if no name is given twice
        outNames = set([task["name"] for task in self._tasks])

        if len(outNames) != len(self._tasks):
            print("List of task names")
            pprint.pprint([task["name"] for task in self._tasks])

            raise RuntimeError("Each job has to have a unique name! See list of name above.")

        ##Check if no dir is given twice
        outDirs = set([task["dir"] for task in self._tasks])

        if len(outDirs) != len(self._tasks):
            print("List of task directories")
            pprint.pprint([task["dir"] for task in self._tasks])

            raise RuntimeError("Each job has to have a unique directory! See list of dirs above.")

        ##Create dependency graph
        self.__sortTasks()
               
        nFinished = 0

        nCores = cpu_count()
        pool = Pool(processes=nCores)
        localJobs = {}
        condorJobs = []

        while nFinished != len(self._tasks):
            self.__printRunStatus(time.time() - self.startTime)
            self.webpage.createWebpage(self._tasks, self.workDir)

            for task in self._tasks:
                if task["status"] == "FINISHED":
                    continue

                ##Skip task if already finished and you dont want to rerun
                if self.checkOutput:
                    if task.checkOutput():
                        task["status"] = "FINISHED"
                        nFinished+=1
                        continue

                if True in [t["status"] != "FINISHED" for t in task.dependencies]:
                    continue

                if not task.isPrepared:
                    task.prepare()

                if task["run-mode"] == "Local":
                    if len(localJobs) < nCores and task not in localJobs:
                        task["status"] = "RUNNING"
                        localJobs[task] = pool.apply_async(task)

                    if task not in localJobs:
                        continue

                    if localJobs[task].ready():
                        if localJobs[task].get() == 0:
                            task["status"] = "FINISHED"
                            localJobs.pop(task)
                            nFinished+=1

                        else:
                            task["status"] = "FAILED"
                            raise RuntimeError("Local job failed: {}. For error code look in {}/err.txt".format(task["name"], task["dir"]))

                if task["run-mode"] == "Condor":
                    if task["status"] == "VALID":
                        condorJobs.append(task)
                        continue

                    with open("{}/log.txt".format(task["dir"])) as logFile:
                        condorLog = logFile.readlines()

                    if True in ["Job executing" in line for line in condorLog]:
                        task["status"] = "RUNNING"

                    if True in ["Normal termination" in line for line in condorLog]:
                        if True in ["(return value 0)" in line for line in condorLog]:
                            task["status"] = "FINISHED"
                            nFinished+=1
                            continue

                        else:
                            task["status"] = "FAILED"
                            raise RuntimeError("Condor job failed: {}. For error code look in {}/err.txt".format(task["name"], task["dir"]))

                    if True in ["SYSTEM_PERIODIC_REMOVE" in line for line in condorLog]:
                        task["status"] = "VALID"
                        task["run-mode"] = "Local"

            if condorJobs:
                self.__submitCondor(condorJobs)
                condorJobs = []
