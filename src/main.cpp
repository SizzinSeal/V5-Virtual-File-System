/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       LemLib Team                                               */
/*    Created:      3/23/2023, 12:18:22 PM                                    */
/*    Description:  LemLib file system serial interpreter                     */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string.h>

// Exception codes:
// VFS_INIT_FAILED
// FILE_NOT_FOUND
// FILE_ALREADY_EXISTS
// CANNOT_OPEN_FILE

typedef struct VFS_INIT_FAILED {} VFS_INIT_FAILED;

typedef struct FILE_NOT_FOUND {} FILE_NOT_FOUND;

typedef struct FILE_ALREADY_EXISTS {} FILE_ALREADY_EXISTS;

typedef struct CANNOT_OPEN_FILE {} CANNOT_OPEN_FILE;

/**
 * @brief Structure for an entry in the index file
 *
 * @param name the name of the file
 * @param sector the sector the file is stored in
 */
typedef struct lemlibFile {
    std::string name;
    std::string sector;
} lemlibFile;

/**
 * @brief Initialize the file system
 *
 */
void initVFS() {
    // Check if the index file exists
    std::ifstream indexFile;
    indexFile.open("index.txt");
    // If the index file does not exist, create it
    if (!indexFile.is_open()) {
        std::ofstream indexFile;
        indexFile.open("index.txt");
        // throw an exception if the index file could not be created
        if (!indexFile.is_open()) throw VFS_INIT_FAILED();
        indexFile.close();
    }
}

/**
 * @brief Read the index file
 *
 * @return std::vector<lemlibFile> contents of the index file
 */
std::vector<lemlibFile> readFileIndex() {
    std::vector<lemlibFile> index;
    // open the index file
    std::ifstream indexFile;
    indexFile.open("index.txt");
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE();
    // iterate through the index file
    for (std::string line; std::getline(indexFile, line);) {
        // the number after the last backslash is the sector
        std::string name = line.substr(0, line.find_last_of("/"));
        std::string sector = line.substr(line.find_last_of("/") + 1);
        // push the file to the index
        index.push_back({name, sector});
    }
    indexFile.close();
    return index;
}

/**
 * @brief Get the Sector object
 *
 * @param path the path of the virtual file
 * @return std::string the sector the file is stored in, or null if the file is not found
 */
std::string getFileSector(std::string path) {
    if (path.find("/") != 0) path = "/" + path;
    // get the file index
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through files in the index
    for (const lemlibFile& file : index) {
        // Check if the name matches
        if (file.name == path) return file.sector;
    }
    // Return empty string if the file is not found
    return "";
}

/**
 * @brief List all the files and folders in a directory
 *
 * @param dir the directory to list
 * @return std::vector <std::string> a vector of all the files and folders in the directory
 */
std::vector<std::string> listDirectory(std::string dir, bool recursive = false) {
    if (dir.find("/") != 0) dir = "/" + dir;
    std::vector<std::string> files;
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile line : index) {
        // Check if the name starts with the directory
        if (line.name.find(dir) == std::string::npos) continue;
        // remove the directory from the name
        std::string name = line.name.substr(line.name.find(dir) + strlen(dir.c_str()));
        // if there is a remaining slash, a directory is found
        if (name.find("/") != std::string::npos && !recursive) name = name.substr(0, name.find("/")) + "/";
        // push back the name, if it is not already in the vector
        bool found = false;
        for (const std::string file : files) {
            if (file == name) {
                found = true;
                break;
            }
        }
        if (!found) files.push_back(name);
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
bool fileExists(std::string path) {
    if (path.find("/") != 0) path = "/" + path;
    // Iterate through the index
    std::vector<lemlibFile> index = readFileIndex();
    for (const lemlibFile file : index) {
        // Check if the name matches
        if (file.name == path) return true;
    }
    return false;
}

/**
 * @brief delete a virtual file
 *
 * @param path the path of the virtual file
 */
void deleteFile(std::string path) {
    if (path.find("/") != 0) path = "/" + path;
    if (!fileExists(path)) throw FILE_NOT_FOUND();
    // empty the sector the file is stored in
    std::ofstream sector;
    sector.open(getFileSector(path.c_str()));
    sector << "";
    sector.close();
    // remove the file from the index file
    std::vector<lemlibFile> index = readFileIndex();
    std::ofstream indexFile;
    indexFile.open("index.txt");
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE();
    for (const lemlibFile line : index) {
        if (line.name != path) indexFile << line.name << "/" << line.sector << std::endl;
    }
    indexFile.close();
}

/**
 * @brief Create a virtual file
 *
 * @param path the path of the virtual file
 * @return std::string the sector the file is stored in
 */
std::string createFile(std::string path, bool overwrite = true) {
    if (path.find("/") != 0) path = "/" + path;
    // Create the file in the index file
    std::ofstream indexFile;
    indexFile.open("index.txt", std::ios_base::app);
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE();
    // Check if the file already exists
    if (fileExists(path)) {
        if (overwrite) deleteFile(path);
        else throw FILE_ALREADY_EXISTS();
    }
    // Find the first empty sector
    std::vector<lemlibFile> index = readFileIndex();
    int sector = 0;
    for (const lemlibFile file : index) {
        if (file.sector == std::to_string(sector)) sector++;
    }
    // Create the file
    indexFile << path << "/" << sector << std::endl;
    indexFile.close();
    // create the sector file
    std::ofstream sectorFile;
    sectorFile.open(std::to_string(sector));
    if (!sectorFile.is_open()) throw CANNOT_OPEN_FILE();
    sectorFile << "";
    sectorFile.close();
    return std::to_string(sector);
}

/**
 * @brief Main function
 *
 * @return int program exit code
 */
int main() {
    // Initialize the file system
    initVFS();
    std::cout << "[INIT] Initialized" << std::endl;
}
