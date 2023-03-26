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
    // Initialize the vector
    std::vector<lemlibFile> index;
    // Open the index file
    std::ifstream indexFile;
    indexFile.open("index.txt");
    // throw an exception if the index file could not be opened
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE();
    // iterate through the index file
    for (std::string line; std::getline(indexFile, line);) {
        // split the line into the name and the sector
        // the number after the last backslash is the sector
        std::string name = line.substr(0, line.find_last_of("/"));
        std::string sector = line.substr(line.find_last_of("/") + 1);
        // add the file to the index
        index.push_back({name, sector});
    }
    return index;
}

/**
 * @brief Get the Sector object
 *
 * @param path the path of the virtual file
 * @return const char* the sector the file is stored in, or null if the file is not found
 */
const char* getFileSector(const char* path) {
    std::string filePath = path;
    // if the path does not start with a slash, add one
    if (filePath.find("/") != 0) filePath = "/" + filePath;
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile& file : index) {
        // Check if the name matches
        if (file.name == filePath) return file.sector.c_str();
    }
    // Return null if the file is not found
    return NULL;
}

/**
 * @brief List all the files and folders in a directory
 *
 * @param dir the directory to list
 * @return std::vector <std::string> a vector of all the files and folders in the directory
 */
std::vector<std::string> listDirectory(const char* dir, bool recursive = false) {
    std::string directory = dir;
    // if the path does not start with a slash, add one
    if (directory.find("/") != 0) directory = "/" + directory;
    // Initialize the vector
    std::vector<std::string> files;
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile& line : index) {
        // Check if the name starts with the directory
        if (line.name.find(directory) == std::string::npos) continue;
        // remove the directory from the name
        std::string name = line.name.substr(line.name.find(directory) + strlen(directory.c_str()));
        // if there is a remaining slash, a directory is found
        if (name.find("/") != std::string::npos && !recursive) name = name.substr(0, name.find("/")) + "/";
        // push back the name, if it is not already in the vector
        bool found = false;
        for (const std::string& file : files) {
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
bool fileExists(const char* path) {
    std::string filePath = path;
    // if the path does not start with a slash, add one
    if (filePath.find("/") != 0) filePath = "/" + filePath;
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile& file : index) {
        // Check if the name matches
        if (file.name == filePath) return true;
    }
    return false;
}

/**
 * @brief Check if a file exists
 *
 * @param path path of the file
 * @return true the file exists
 * @return false the file does not exist
 */
bool fileExists(std::string path) { return fileExists(path.c_str()); }

/**
 * @brief delete a virtual file
 *
 * @param path the path of the virtual file
 */
void deleteFile(const char* path) {
    std::string filePath = path;
    // if the path does not start with a slash, add one
    if (filePath.find("/") != 0) filePath = "/" + filePath;
    // check if the file exists
    if (!fileExists(filePath)) throw FILE_NOT_FOUND();
    // empty the sector the file is stored in
    std::ofstream sector;
    sector.open(getFileSector(filePath.c_str()));
    sector << "";
    sector.close();
    // remove the file from the index file
    std::vector<lemlibFile> index = readFileIndex();
    std::ofstream indexFile;
    indexFile.open("index.txt");
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE();
    for (const lemlibFile& line : index) {
        if (line.name != filePath) indexFile << line.name << "/" << line.sector << std::endl;
    }
    indexFile.close();
}

/**
 * @brief delete a virtual file
 *
 * @param path the path of the virtual file
 */
void deleteFile(std::string path) { deleteFile(path.c_str()); }

/**
 * @brief Create a virtual file
 *
 * @param path the path of the virtual file
 * @return const char* the sector the file is stored in
 */
const char* createFile(const char* path, bool overwrite = true) {
    std::string filePath = path;
    // if the path does not start with a slash, add one
    if (filePath.find("/") != 0) filePath = "/" + filePath;
    // Create the file in the index file
    std::ofstream indexFile;
    indexFile.open("index.txt", std::ios_base::app);
    if (!indexFile.is_open()) throw CANNOT_OPEN_FILE();
    // Check if the file already exists
    if (fileExists(filePath)) {
        // If the file should be overwritten, delete the file
        if (overwrite) deleteFile(filePath);
        // Otherwise, throw an exception
        else throw FILE_ALREADY_EXISTS();
    }
    // Find the first empty sector
    std::vector<lemlibFile> index = readFileIndex();
    int sector = 0;
    for (const lemlibFile& file : index) {
        if (file.sector == std::to_string(sector)) sector++;
    }
    // Create the file
    indexFile << filePath << "/" << sector << std::endl;
    indexFile.close();
    // create the sector file
    std::ofstream sectorFile;
    // check if the sector file was created
    if (!sectorFile.is_open()) throw CANNOT_OPEN_FILE();
    sectorFile.open(std::to_string(sector));
    sectorFile << "";
    sectorFile.close();
    return std::to_string(sector).c_str();
}

/**
 * @brief Create a virtual file
 *
 * @param path the path of the virtual file
 * @return const char* the sector the file is stored in
 */
const char *createFile(std::string path, bool overwrite = true) { return createFile(path.c_str(), overwrite); }


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
