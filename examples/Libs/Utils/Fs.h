#pragma once
#include <vector>
#include <string>
#include <filesystem>

namespace qe::Examples::Fs
{
    using Path = std::filesystem::path;

    std::string LoadTextFile(const std::string& filePath);
    std::vector<uint8_t> LoadBinaryFile(const std::string& filePath);
    void SaveTextFile(const std::string& filePath, const std::string& content);
    void WriteBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data);
    void WriteHexFile(const std::string& filePath, const std::vector<uint8_t>& data);
}

