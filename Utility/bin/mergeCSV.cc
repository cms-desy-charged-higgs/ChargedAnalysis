#include <experimental/filesystem>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/frame.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    Parser parser(argc, argv);

    std::vector<std::string> labels = parser.GetVector<std::string>("labels");
    std::string sort = parser.GetValue<std::string>("sort");
    std::string dir = parser.GetValue<std::string>("dir");
    std::string outName = parser.GetValue<std::string>("out-name");

    Frame frame(labels);

    for (const std::experimental::filesystem::directory_entry& entry : std::experimental::filesystem::directory_iterator(dir)){
        frame.ReadCSV(entry.path());

        std::cout << "Read file: " << entry.path() << std::endl;

        std::experimental::filesystem::remove(entry.path());
    }

    frame.Sort(sort, false);
    frame.WriteCSV(dir + "/" + outName + ".csv");

    std::cout << "Merged file: " << dir + "/" + outName + ".csv" << std::endl;
}
