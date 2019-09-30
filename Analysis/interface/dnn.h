#ifndef DNN_H
#define DNN_H

#include <string>
#include <vector>

#include <Python.h>

class DNN{
    private:
        //Python C Objects needed
        PyObject* module;
        PyObject* moduleClass;
        PyObject* modelInstance;
        PyObject* predictFunction;

    public:
        DNN();
        ~DNN();

        void SetModel(const std::string& importPath, const std::string& className, const std::string& modelPath);
        float Predict();
};

#endif
