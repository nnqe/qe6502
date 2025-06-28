#pragma once
#include <vector>
#include <string>

namespace qe::Examples::Fs
{
    std::string LoadTextFile(const std::string& filePath);
    std::vector<uint8_t> LoadBinaryFile(const std::string& filePath);
}

