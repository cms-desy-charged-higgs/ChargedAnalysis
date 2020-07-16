#ifndef TREEPARSER
#define TREEPARSER

#include <TH1.h>

#include <ChargedAnalysis/Analysis/include/treefunction.h>

/**
* @brief Class for interpreting strings from which information for TreeFunction class execution 
*
* This class interprets custom strings, which will be used to configure instances of the TreeFunction class.
* 
* Example for custom strings:
*
* -# __f:n=Pt/p:n=e,i=1,wp=l/h:nxb=20,xl=20,xh=200__ (for producing histograms)
* -# __f:n=N/p:n=j/c:n=bigger,v=3f:n=N/p:n=j/c:n=bigger,v=3__ (for cutting)  
*/

class TreeParser{
    public:
        /**
        * @brief Default constructor
        */
        TreeParser();

        /**
        * @brief Extract the 'f:' function key word from custrom string
        *
        * @param[in] parameter Custom string with TreeFunction configuration
        * @param[in] func Treefunction instance by reference, where TreeFunction::SetFunction will be invoked
        */
        void GetFunction(const std::string& parameter, TreeFunction& func);

        /**
        * @brief Extract the 'p:' function key word from custrom string
        *
        * @param[in] parameter Custom string with TreeFunction configuration
        * @param[in] func Treefunction instance by reference, where TreeFunction::SetFunction will be invoked
        */
        void GetParticle(const std::string& parameter, TreeFunction& func);

        /**
        * @brief Extract the 'c:' function key word from custrom string
        *
        * @param[in] parameter Custom string with TreeFunction configuration
        * @param[in] func Treefunction instance by reference, where TreeFunction::SetFunction will be invoked
        */
        void GetCut(const std::string& parameter, TreeFunction& func);

        /**
        * @brief Extract the 'h:' function key word from custrom string
        *
        * @param[in] parameter Custom string with TreeFunction configuration
        * @param[in] hist ROOT histogram, where TH1::SetBinning will be invoked
        */
        void GetBinning(const std::string& parameter, TH1* hist);        
};

#endif


