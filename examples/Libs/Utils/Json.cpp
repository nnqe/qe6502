#include "Json.h"
#include <fmt/format.h>
#include "Fs.h"

namespace qe::Examples
{

Json LoadJsonFile(const std::string &jsonFile)
{
    auto fileStr = Fs::LoadTextFile( jsonFile );
    try
    {
        Json tests = Json::parse( fileStr );
        return tests;
    }
    catch(const std::exception& e)
    {
        fmt::println("Loading JSON ERROR: file: {}", fileStr);
        fmt::println("Loading JSON ERROR: filename: {}", jsonFile);
        fmt::println("Loading JSON ERROR: exception: {}", e.what());
        exit(1);
    }
    return {};
}

}
