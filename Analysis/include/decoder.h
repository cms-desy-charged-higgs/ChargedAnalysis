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

        void GetFunction(const std::string& parameter, NTupleReader& reader, const std::experimental::source_location& location = std::experimental::source_location::current());
        void GetParticle(const std::string& parameter, NTupleReader& reader, Weighter*
 weighte = nullptr,  const std::experimental::source_location& location = std::experimental::source_location::current());
        void GetBinning(const std::string& parameter, TH1* hist, const std::experimental::source_location& location = std::experimental::source_location::current());
        void GetCut(const std::string& parameter, NTupleReader& reader, const std::experimental::source_location& location = std::experimental::source_location::current());

};

#endif
