/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       LemLib Team                                               */
/*    Created:      3/23/2023, 12:18:22 PM                                    */
/*    Description:  LemLib Virtual File System                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include <exception>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string.h>
#include <algorithm>

#if defined VEXV5
#define PREFACE ""
#else
#define PREFACE "/usd/"
#endif

// Exception codes
#define VFS_NOT_INITIALIZED VFSException("VFS_NOT_INITIALIZED")
#define VFS_INIT_FAILED VFSException("VFS_INIT_FAILED")
#define FILE_NOT_FOUND(filename) (VFSException(std::string("FILE_NOT_FOUND (") + filename + ")"))
#define FILE_ALREADY_EXISTS(filename) (VFSException(std::string("FILE_ALREADY_EXISTS (") + filename + ")"))
#define CANNOT_OPEN_FILE(filename) (VFSException(std::string("CANNOT_OPEN_FILE (") + filename + ")"))

/**
 * @brief Exception class for the VFS
 *
 */
class VFSException : public std::exception {
    public:
        /**
         * @brief Construct a new VFSException
         *
         * @param message the message to display when the exception is thrown
         */
        VFSException(const std::string& message) : m_message(message) {}

        /**
         * @brief Get the message of the exception
         *
         * @return const char* the message
         */
        const char* what() const noexcept override { return m_message.c_str(); }
    private:
        std::string m_message;
};

/**
 * @brief Structure for an entry in the index file
 *
 * @param name the name of the file
 * @param sector the sector the file is stored in
 */
typedef struct lemlibFile {
        std::string name;
        std::string sector;

        lemlibFile(const std::string& name_, const std::string& sector_) : name(name_), sector(sector_) {}
} lemlibFile;

/**
 * @brief Initialize the file system
 *
 */
void initVFS() {
    // Check if the index file exists
    std::ifstream indexFile("/usd/index.txt");
    // If the index file does not exist, create it
    if (!indexFile.is_open()) {
        std::ofstream newIndexFile("/usd/index.txt");
        // throw an exception if the index file could not be created
        if (!newIndexFile.is_open()) throw VFS_INIT_FAILED;
        indexFile.close();
    }
}

/**
 * @brief Read the index file
 *
 * @return std::vector<lemlibFile> contents of the index file
 */
std::vector<lemlibFile> readFileIndex() {
    // iterate through the index file
    std::ifstream indexFile("/usd/index.txt");
    if (!indexFile) throw CANNOT_OPEN_FILE("/usd/index.txt");
    std::vector<lemlibFile> index;
    for (std::string line; std::getline(indexFile, line);) {
        const size_t last_slash_pos = line.find_last_of("/");
        // split the line into the name and sector. The number after the last slash is the sector number
        index.emplace_back(line.substr(0, last_slash_pos), line.substr(last_slash_pos + 1));
    }
    return index;
}

/**
 * @brief Get the sector of a virtual file
 *
 * @param path the path of the virtual file
 * @return std::string the sector the file is stored in, or null if the file is not found
 */
std::string getFileSector(const std::string& path) {
    // If the path does not start with a slash, add one
    const std::string corrected_path = (path.front() == '/') ? path : ('/' + path);
    // Iterate through the index
    const std::vector<lemlibFile> index = readFileIndex();
    const std::vector<lemlibFile>::const_iterator it =
        std::find_if(index.begin(), index.end(), [&](const lemlibFile& file) { return file.name == corrected_path; });
    // return the sector if the file is found, or an empty string if it is not found
    return (it != index.end()) ? it->sector : "";
}

/**
 * @brief List all the files and folders in a directory
 *
 * @param dir the directory to list
 * @return std::vector <std::string> a vector of all the files and folders in the directory
 */
std::vector<std::string> listDirectory(const std::string& dir, bool recursive = false) {
    // If the path does not start with a slash, add one
    const std::string corrected_dir = (dir.front() == '/') ? dir : ('/' + dir);
    std::vector<std::string> files;
    // Iterate through all the files in the index
    for (const lemlibFile& line : readFileIndex()) {
        // Check if the name starts with the directory
        if (line.name.find(corrected_dir) != 0) continue;
        // Remove the directory from the name
        std::string name = line.name.substr(corrected_dir.length());
        // If there is a remaining slash and recursion is disabled, only keep the name of the directory
        if (name.find("/") != std::string::npos && !recursive) name = name.substr(0, name.find("/") + 1);
        // Add the name to the vector if it is not already present
        if (std::find(files.begin(), files.end(), name) == files.end()) files.push_back(name);
    }
    return files;
}

/**
 * @brief Check if a file exists
 *
 * @param path path of the file
 * @return true the file exists
 * @return false the file does not exist
 */
bool fileExists(const std::string& path) {
    // if the path does not start with a slash, add one
    const std::string corrected_path = (path.front() == '/') ? path : ('/' + path);
    // return true if the file is found in the index, false otherwise
    std::vector<lemlibFile> index = readFileIndex();
    return std::any_of(index.begin(), index.end(), [&](const lemlibFile& file) { return file.name == corrected_path; });
}

/**
 * @brief delete a virtual file
 *
 * @param path the path of the virtual file
 */
void deleteFile(const std::string& path) {
    // if the path does not start with a slash, add one
    const std::string corrected_path = (path.front() == '/') ? path : ('/' + path);
    if (!fileExists(corrected_path)) throw FILE_NOT_FOUND(corrected_path);
    // empty the sector the file is stored in
    std::ofstream sector(getFileSector(corrected_path.c_str()));
    sector << "";
    // remove the file from the index file
    auto index = readFileIndex();
    index.erase(
        std::remove_if(index.begin(), index.end(), [&](const lemlibFile& line) { return line.name == corrected_path; }),
        index.end());
    std::ofstream indexFile("/usd/index.txt");
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE("/usd/index.txt");
    for (const lemlibFile& line : index) { indexFile << line.name << "/" << line.sector << std::endl; }
}

/**
 * @brief Create a virtual file
 *
 * @param path the path of the virtual file
 * @return std::string the sector the file is stored in
 */
std::string createFile(const std::string& path, bool overwrite = true) {
    // if the path does not start with a slash, add one
    const std::string corrected_path = (path.front() == '/') ? path : ('/' + path);
    // Create the file in the index file
    std::ofstream indexFile("/usd/index.txt", std::ios_base::app);
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE("/usd/index.txt");
    // Check if the file already exists
    if (fileExists(corrected_path)) {
        if (overwrite) deleteFile(corrected_path);
        else throw FILE_ALREADY_EXISTS(corrected_path);
    }
    // Find the first empty sector
    std::vector<lemlibFile> index = readFileIndex();
    int sector = 0;
    for (const lemlibFile& file : index) {
        if (file.sector == std::to_string(sector)) sector++;
    }
    // Create the file
    indexFile << corrected_path << "/" << sector << std::endl;
    indexFile.close();
    // create the sector file
    std::ofstream sectorFile("/usd" + std::to_string(sector));
    if (!sectorFile.is_open()) throw CANNOT_OPEN_FILE("/usd" + std::to_string(sector));
    sectorFile << "";
    sectorFile.close();
    // return the sector the file is stored in
    return std::to_string(sector);
}
