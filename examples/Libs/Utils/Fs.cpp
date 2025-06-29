#include "Fs.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace qe::Examples::Fs
{

std::string LoadTextFile(const std::string &filePath)
{
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<uint8_t> LoadBinaryFile(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer((size_t(fileSize)));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        throw std::runtime_error("Failed to read file: " + filePath);
    }

    return buffer;
}


void WriteBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data)
{
    // Open file in binary mode, truncating existing content
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }

    // Write the entire buffer to disk
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file)
    {
        throw std::runtime_error("Failed to write data to file: " + filePath);
    }

    // Optional: flush to ensure all data is written
    file.flush();
    if (!file)
    {
        throw std::runtime_error("Failed to flush data to file: " + filePath);
    }

    // File closes automatically when `file` goes out of scope
}

void WriteHexFile(const std::string& filePath, const std::vector<uint8_t>& data)
{
    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    file << "{";
    for (size_t i = 0; i < data.size(); ++i)
    {
        if (i > 0)
            file << ",";

        if ((i % 32) == 0) file << "\n";

        file << "0x"
             << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(data[i]);
    }
    file << "\n};";
    file.flush();
    if (!file)
    {
        throw std::runtime_error("Failed to flush data to file: " + filePath);
    }
}


}
