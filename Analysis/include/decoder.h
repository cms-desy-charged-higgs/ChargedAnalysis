#ifndef DECODER_H
#define DECODER_H

#include <string>
#include <experimental/source_location>

#include <TH1.h>

#include <ChargedAnalysis/Analysis/include/ntuplereader.h>
#include <ChargedAnalysis/Analysis/include/weighter.h>

class Decoder{
    public:
        Decoder();

        bool hasYInfo(const std::string& parameter);

        void GetFunction(const std::string& parameter, NTupleFunction& func, const bool& readY = false, const std::experimental::source_location& location = std::experimental::source_location::current());
        void GetParticle(const std::string& parameter, NTupleFunction& func, const bool& readY = false, const std::experimental::source_location& location = std::experimental::source_location::current());
        void GetParticle(const std::string& parameter, NTupleReader& reader, Weighter& weight, const bool& readY = false, const std::experimental::source_location& location = std::experimental::source_location::current());
        void GetBinning(const std::string& parameter, TH1* hist, const bool& isY = false, const std::experimental::source_location& location = std::experimental::source_location::current());
        void GetCut(const std::string& parameter, NTupleFunction& func, const std::experimental::source_location& location = std::experimental::source_location::current());

};

#endif
