#pragma once
#include <nlohmann/json.hpp>

namespace qe::Examples
{
    using Json = nlohmann::json;
    Json LoadJsonFile(const std::string& jsonFile);
}
