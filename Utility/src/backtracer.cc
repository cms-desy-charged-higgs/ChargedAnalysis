#include <ChargedAnalysis/Utility/include/backtracer.h>

Backtracer::Backtracer(){
    for(int sig = 0; sig < kMAXSIGNALS; sig++) gSystem->ResetSignal((ESignals)sig);

    std::signal(SIGSEGV, Backtracer::SigHandler);
    std::set_terminate(Backtracer::SigHandlerForException);
}

std::string Backtracer::GetLibAddress(const std::string& libName, const std::string& mangledName, const std::string& addrShiftStr){
    //Use nm to get address for .so file
    std::string command = "> /tmp/nm.txt; nm " + libName + " | grep " + mangledName + " >> /tmp/nm.txt";

    //Call popen, catch output and read first line which contains the information
    std::system(command.c_str());

    std::ifstream result("/tmp/nm.txt");
    std::string line;

    if(result.is_open()) std::getline(result, line);

    //Split read line by " ", the first item give the address
    std::stringstream baseAddrStream, addrShiftStream;
    baseAddrStream << StrUtil::Split(line, " ").at(0);
    addrShiftStream << addrShiftStr;

    //Calculate address from base address and shift in hex
    long addrShift, baseAddr;
    baseAddrStream >> std::hex >> baseAddr;
    addrShiftStream >> std::hex >> addrShift;

    //Return result as string in hex representation
    std::stringstream address;
    address << std::hex << baseAddr + addrShift;

    return address.str();
}

std::pair<std::string, std::string> Backtracer::GetFileAndLine(const std::string& libName, const std::string& address){
    //Use addr2line to get line/file for .so file
    std::string command = "> /tmp/addr.txt; addr2line -e " + libName + " -i " + address + " >> /tmp/addr.txt";

    //Call popen, catch output and read first line which contains the information
    std::system(command.c_str());

    std::ifstream result("/tmp/addr.txt");
    std::string l, line;

    if(result.is_open()){
        while(std::getline(result, l)){
            line = l;
        }
    }

    //Split output for filename and line number
    std::string fileName, lineNumber;

    fileName = StrUtil::Split(line, ":").at(0);
    lineNumber = StrUtil::Split(line, ":").at(1);

    if(!StrUtil::Find(lineNumber, " ").empty()){
        lineNumber = StrUtil::Split(lineNumber, " ").at(0);
    }

    return {fileName, lineNumber};
}

void Backtracer::SigHandler(int sig){
    void* array[128];
	int size = backtrace(array, 128);
	char** strings = backtrace_symbols(array, size);

    //Header
    std::cout << std::endl;
    std::cout << "An segfault occured or an exception is thrown. See for more information the backtrace.";
    std::cout << std::endl << std::endl;

	for(unsigned int i = 0; i < size; ++i){
        std::string s(strings[i]);

        std::size_t start = StrUtil::Find(s, "(").at(0) + 1;

        std::size_t end = 0, ptrEnd = 0;;    
        if(!StrUtil::Find(s, "+").empty()){
            end = StrUtil::Find(s.substr(start), "+").at(0);
            ptrEnd = StrUtil::Find(s.substr(start+end), ")").at(0) - 1;
        }

        std::string mangledName = s.substr(start, end);
        std::string libName = s.substr(0, start - 1);
        std::string addrShift = s.substr(start + end + 1, ptrEnd);

        if(end != 0){
            std::string s = Backtracer::GetLibAddress(libName, mangledName, addrShift);
            std::string fileName, lineNumber;
            std::tie(fileName, lineNumber) = Backtracer::GetFileAndLine(libName, s);
            std::string demangledName;

            int status;
            char* demangled = abi::__cxa_demangle(mangledName.c_str(), 0, 0, &status);
            if(demangled != 0) demangledName = demangled;
            else demangledName = mangledName;
            delete demangled;

            std::cout << i + 1 << "." << std::endl;
            std::cout << "---------------" << std::endl;

            std::cout << "\033[4mFILE\033[0m " << fileName << std::endl << std::endl;
            std::cout << "\033[4mFUNCTION\033[0m " << demangledName << std::endl << std::endl;
            std::cout << "\033[4mLINE\033[0m " << lineNumber << std::endl << std::endl;
        }
    }

    delete strings;

    std::exception_ptr eptr = std::current_exception();

    if(eptr != nullptr){
        try{
            std::rethrow_exception(eptr);
        }
        catch(const std::exception& e) {
            std::cout << "\nException leading to the termination: ";
            std::cout << e.what() << std::endl;
        }
    }

    std::exit(1);

    //https://stackoverflow.com/questions/18892033/analyze-backtrace-of-a-crash-occurring-due-to-a-faulty-library
    // https://stackoverflow.com/questions/7556045/how-to-map-function-address-to-function-in-so-files
}

void Backtracer::SigHandlerForException(){
    Backtracer::SigHandler(1);
}
