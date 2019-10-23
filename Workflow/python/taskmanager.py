import yaml
import copy
import os
import time
import sys
import subprocess

from multiprocessing import Pool, cpu_count

class TaskManager(object):
    def __init__(self):
        ##Header of worflow HTML
        self._htmlHead = """
            <!DOCTYPE html>
            <html>
                <head>
                    <style>
                        html, body {height : 100%}

                        div {font-size: 70%; position:absolute; border-radius: 10px; border-style: solid; border-width: 1px; box-shadow: 5px 5px 2px grey;}
                        div.task {text-align: center; width: 250px; height: 28px;}
                        div.yaml {padding:10px; display: none; top: 0px; left: 0px; width: auto; height: auto; z-index: 1; background-color: white}
                        
                        svg {width: 100%; height: 100%; top:0; left:0; position: absolute;}
                        object {width: 200px; height: 1000px;}

                    </style>
                    <script type="text/javascript" src="http://livejs.com/live.js"></script>

                </head>
            """

        ##Body of worflow HTML
        self._htmlBody = """
                    <div class='yaml' onmouseover='showYaml(this)' onmouseout='hideYaml()'></div>

                    <svg>
                        {lines}
                    </svg>

                    {divs}

                    <script>
                        document.getElementsByTagName('svg')[0].style.height = '{maxheight}px'
        """

        ##Javascript functions of worflow HTML
        self._javascript = """
                    function showYaml(x){
                        var divYaml = document.getElementsByClassName("yaml")[0];
                        divYaml.style.display = 'block';

                        if(x.className != "yaml"){
                            divYaml.style.top = x.style.top;
                            divYaml.style.left = parseInt(x.style.left) + 260 + "px";
                            divYaml.innerHTML = x.getAttribute("data-yaml");
                        }
                    }

                    function hideYaml(){
                        var divYaml = document.getElementsByClassName('yaml')[0];
                        divYaml.style.display = 'none';
                    }
                    </script>
                </body>
            </html>
            """

        ##Templates for divs and lines for the task and their dependencies
        self._divTmp = "<div class='task' style='left:{x}px; top:{y}px; background:{color};' onmouseover='showYaml(this)' data-yaml='{yaml}'> {name} </div>"
        self._lineTmp = "<line x1='{x1}px' y1='{y1}px' x2='{x2}px' y2='{y2}px' stroke='gray'></line>" 

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


    def __printRunStatus(self, layer, time):
        ##Helper function:
        nStatus = lambda l, status: len([1 for task in l if task["status"] == status])

        self.localRun = nStatus(self.localTask, "RUNNING")
        self.localFin = nStatus(self.localTask, "FINISHED")
        self.localFail = nStatus(self.localTask, "FAILED")

        self.condRun = nStatus(self.condorTask, "RUNNING")
        self.condFin = nStatus(self.condorTask, "FINISHED")
        self.condFail = nStatus(self.condorTask, "FAILED")

        runString = "\t\t\033[92mRUN\033[0m : {} \t\033[93mVALID\033[0m :  {} \t\033[91mFAILED\033[0m : {} \t\033[1mFINISHED\033[0m : {}"

        statString = "{}/{} ({} %) \t Time : {} s".format(self.localFin+self.condFin , len(self.localTask)+len(self.condorTask), 100*(self.localFin+self.condFin)/(len(self.localTask)+len(self.condorTask)), time)

        os.system("clear")

        outputPrint = [
                "Overview of task layer {}".format(layer),
                "".join(["-" for i in range(80)]),
                "Overall stats",
                "",
                statString,
                "".join(["-" for i in range(80)]),
                "Local Task",
                "",
                runString.format(self.localRun, len(self.localTask) - self.localFin - self.localRun - self.localFail, self.localFail, self.localFin),
                "".join(["-" for i in range(80)]),
                "Condor Task",
                "",
                runString.format(self.condRun, len(self.condorTask) - self.condFin - self.condRun - self.condFail, self.condFail, self.condFin),
                "".join(["-" for i in range(80)]),
        ]

        for line in outputPrint:
            print(line.center(os.get_terminal_size().columns))
        
    def drawGraph(self):
        divPos = {}
        maxheight = 0.

        divs = []
        yamls = []
        lines = []

        statusToColor = {
                        "VALID": "linear-gradient(to bottom right, #f4ff00, #e8fb15, #dcf821, #d1f42a, #c6f032)",
                        "SUBMITTED": "linear-gradient(to bottom right, #ff8500, #f38900, #e68c00, #da8f00, #cf9106)",
                        "RUNNING": "linear-gradient(to bottom right, #00ccff 0%, #3333cc 111%)",
                        "FINISHED": "linear-gradient(to bottom right, #00ff00 0%, #009933 111%)",
                        "FAILED": "linear-gradient(to bottom right, #ff0000 0%, #cc0066 111%)",
        }

        ##Wrap all keys in block to make them colored
        def dicToNiceHtml(dic):
            newDic = {}
            for (key, value) in dic.items():
                if type(value) != dict:
                    newDic["<b style='color:brown'>{}</b>".format(key)] = value

                else:
                    newDic["<b style='color:brown'>{}</b>".format(key)] = dicToNiceHtml(value)

            return newDic

        for (layerIndex, taskLayer) in enumerate(self._graph):
            for index, task in enumerate(taskLayer):
                ##Translate task config into yaml like string
                yamlInfo = yaml.dump(dicToNiceHtml(task), default_flow_style=False, width=float("inf"), indent=4).replace("\n", "<br>").replace("\'", "").replace("    ", "&emsp;&emsp;")

                divPos[task["name"]] = (task["dependencies"], yamlInfo, task["status"], task["display-name"], layerIndex*400, index*40)

                if index*40 + 40 > maxheight:
                    maxheight = index*40 + 40

        ##Fill div/lines templates with right information of the task into list
        for taskName, (dependencies, yamlInfo, status, display, x, y) in divPos.items():
            divs.append(self._divTmp.format(yaml=yamlInfo, x=x, y=y, name = display, color=statusToColor[status]))

            if len(dependencies) != 0:
                for dependency in dependencies: 
                    lines.append(self._lineTmp.format(x1=x, y1=y+12.5, x2=divPos[dependency][4]+250, y2=divPos[dependency][5]+12.5))

        ##Write workflow html
        with open("workflow.html", "w") as f:
            f.write(self._htmlHead + self._htmlBody.format(divs="\n".join(divs), lines = "\n".join(lines),maxheight=maxheight) + self._javascript)

    def runTasks(self):
        ##Helper function:
        nStatus = lambda l, status: len([1 for task in l if task["status"] == status])

        ##Create dependency graph
        self.__createGraph()   

        for layer, taskLayer in enumerate(self._graph):
            self.localTask = []
            self.condorTask = []
            startTime = time.time()

            for task in taskLayer:
                if task["run-mode"] == "Local":
                    task.getDependentFiles(self._graph)
                    self.localTask.append(task)
                  
                if task["run-mode"] == "Condor":
                    task.getDependentFiles(self._graph)
                    self.condorTask.append(task)

            ##Submit condors jobs (Only 200 hundred at one time)
            for index, task in enumerate(self.condorTask):
                task.run()
                task["status"] = "SUBMITTED"

                if index==199:
                    break

            ##Run all local task
            nCores = cpu_count()
            pool = Pool(processes=nCores)
            localRuns = [pool.apply_async(task) for task in self.localTask]

            while(True):
                self.drawGraph() 

                ##Track local jobs
                for index, job in enumerate(localRuns):
                    if job.ready():
                        if job.successful():
                            self.localTask[index]["status"] = "FINISHED"

                        else:
                            self.localTask[index]["status"] = "FAILED"
                            self.__printRunStatus(layer, time.time() - startTime)
                            self.drawGraph()
                            job.get()
                            
                    else:
                        nRuns = nStatus(self.localTask, "RUNNING")
                        nFinished = nStatus(self.localTask, "FINISHED")
                        nFailed = nStatus(self.localTask, "FAILED")
       
                        if nRuns < nCores and nRuns < len(self.localTask) - nFinished - nFailed:
                            self.localTask[index]["status"] = "RUNNING"
                           
                ##Track condor jobs
                for task in self.condorTask:
                    try: #If job not submitted yet, there is no condor.log file
                        with open("{}/condor.log".format(task["condor-dir"])) as logFile:
                            if True in ["Job executing" in line for line in logFile.readlines()]:
                                task["status"] = "RUNNING"

                            logFile.seek(0)

                            if True in ["Normal termination" in line for line in logFile.readlines()]:
                                logFile.seek(0)
                
                                if True in ["(return value 0)" in line for line in logFile.readlines()]:
                                    task["status"] = "FINISHED"

                                else:
                                    task["status"] = "FAILED"

                            logFile.seek(0)

                            if True in ["SYSTEM_PERIODIC_REMOVE" in line for line in logFile.readlines()]:
                                 task["status"] = "VALID"

                    except:
                        pass

                nSubmitted = nStatus(self.condorTask, "SUBMITTED") + nStatus(self.condorTask, "RUNNING")

                ##Submit more jobs up to 200 if jobs left
                for task in self.condorTask:
                    if task["status"] == "VALID":
                        if nSubmitted < 200:
                            task.run()
                            task["status"] = "SUBMITTED"
                            nSubmitted+=1

                ##Print status
                self.__printRunStatus(layer, time.time() - startTime)

                ##Draw graph
                time.sleep(5)
                self.drawGraph()

                if nStatus(self.localTask, "FINISHED") == len(self.localTask) and nStatus(self.condorTask, "FINISHED") == len(self.condorTask):
                    break
