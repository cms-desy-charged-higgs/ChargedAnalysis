#include <ChargedAnalysis/Analysis/interface/dnn.h>
#include <iostream>

DNN::DNN(){}

void DNN::SetModel(const std::string& importPath, const std::string& className, const std::string& modelPath){
    //Import python Keras model and extract class
    module = PyImport_ImportModule("ChargedNetwork.Network.jetmodel");
    moduleClass = PyObject_GetAttrString(module, className.c_str());

    //Generate instance of the class
    PyObject* classArgs = Py_BuildValue("(iiiiff)", 2, 38, 10, 277, 0.09850839085800034, 0.001);
    modelInstance = PyEval_CallObject(moduleClass, classArgs); Py_DECREF(classArgs);

    //Call the load_function of Keras model to load trained model
    PyObject* loadFunction = PyObject_GetAttrString(modelInstance, "load_weights"); 
    PyObject* modelName = Py_BuildValue("(s)", modelPath.c_str());
    PyEval_CallObject(loadFunction, modelName); Py_DECREF(modelName); Py_DECREF(loadFunction);
    
    //Extract predict function as bounded method to call later
    predictFunction = PyObject_GetAttrString(modelInstance, "predict");
}



float DNN::Predict(){
    return 1.;
}

DNN::~DNN(){
    //Clear up
    Py_DECREF(module);
    Py_DECREF(moduleClass); 
    Py_DECREF(modelInstance); 
    Py_DECREF(predictFunction);
}
