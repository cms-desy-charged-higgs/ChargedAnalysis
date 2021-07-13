#ifndef BACKTRACER_H
#define BACKTRACER_H

#include <csignal>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <execinfo.h>
#include <cxxabi.h>

#include <TSystem.h>

#include <ChargedAnalysis/Utility/include/stringutil.h>

class Backtracer{
    private:
        static std::string GetLibAddress(const std::string& libName, const std::string& mangledName, const std::string& addrShiftStr);
        static std::pair<std::string, std::string> GetFileAndLine(const std::string& libName, const std::string& address);

        static void SigHandler(int sig);
        static void SigHandlerForException();

    public:
        Backtracer();
};

#endif
