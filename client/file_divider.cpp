#include "client.h"

// Function to separate file name from file path
void separateFilePath(const string &fullPath, string &filePath, string &fileName)
{
    // Find the last occurrence of the directory separator
    size_t pos = fullPath.find_last_of("/\\");

    if (pos != std::string::npos)
    {
        // Separate the file path and file name
        filePath = fullPath.substr(0, pos);
        fileName = fullPath.substr(pos + 1);
    }
    else
    {
        // If no separator is found, the full path is the file name
        filePath = "";
        fileName = fullPath;
    }
}

// Function to send a particular chunk of file
string sendChunk(string file_full_path, string chunk_hash)
{

    string response = "";
    // Open the file using system calls
    int inputFile = open(file_full_path.c_str(), O_RDONLY);
    if (inputFile == -1)
    {
        cout << "!!ERROR!! Failed to open input file: " + file_full_path << endl;
        return "";
    }

    // Get the file size
    struct stat fileStat;
    if (fstat(inputFile, &fileStat) == -1)
    {
        cout << "!!ERROR!! Failed to get file size" << inputFile << endl;
        return "";
    }
    off_t fileSize = fileStat.st_size;

    // Optimal buffer size for reading chunks
    size_t bufferSize = 512 * 1024; // 512 KB
    // size_t chunkCount = (fileSize + bufferSize - 1) / bufferSize; // Calculate total chunks

    vector<string> fileChunks;

    char *buffer = new char[bufferSize]; // Allocate buffer for reading

    // Read the file in forward order
    for (off_t i = 0; i < fileSize; i += bufferSize)
    {
        size_t chunkSize = min(bufferSize, static_cast<size_t>(fileSize - i));

        // Read the chunk
        if (read(inputFile, buffer, chunkSize) != static_cast<ssize_t>(chunkSize))
        {
            cout << "!!ERROR!! Failed to read from input file" << inputFile;
        }

        // Store the chunk data and compute its SHA1 hash
        string chunkData(buffer, chunkSize);
        string chunkHash = computeSHA1(chunkData);

        if (chunkHash == chunk_hash)
        {
            response = chunkData;
            break;
        }
    }

    // Close the file
    close(inputFile);
    delete[] buffer;
    // cout << "send chunk" << response << endl;
    return response;
}

// Function to compute the SHA1 hash of a given data chunk
string computeSHA1(const string &data)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);

    // Convert hash to hex string
    char hexString[SHA_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(hexString + i * 2, "%02x", hash[i]);
    }
    return string(hexString);
}

// Function to initialize hashes of the file and serialize them
string initializeHashes(string file_full_path, vector<string> &hash)
{

    string response = "";
    // Open the file using system calls
    int inputFile = open(file_full_path.c_str(), O_RDONLY);
    if (inputFile == -1)
    {
        cout << "!!ERROR!! Failed to open input file: " + file_full_path << endl;
        return "";
    }

    // Get the file size
    struct stat fileStat;
    if (fstat(inputFile, &fileStat) == -1)
    {
        cout << "!!ERROR!! Failed to get file size" << inputFile << endl;
        return "";
    }
    off_t fileSize = fileStat.st_size;

    // Optimal buffer size for reading chunks
    size_t bufferSize = 512 * 1024; // 512 KB
    // size_t chunkCount = (fileSize + bufferSize - 1) / bufferSize; // Calculate total chunks

    vector<string> fileChunks;

    char *buffer = new char[bufferSize]; // Allocate buffer for reading

    // Read the file in forward order
    for (off_t i = 0; i < fileSize; i += bufferSize)
    {
        size_t chunkSize = min(bufferSize, static_cast<size_t>(fileSize - i));

        // Read the chunk
        if (read(inputFile, buffer, chunkSize) != static_cast<ssize_t>(chunkSize))
        {
            cout << "!!ERROR!! Failed to read from input file" << inputFile;
        }

        // Store the chunk data and compute its SHA1 hash
        string chunkData(buffer, chunkSize);
        string chunkHash = computeSHA1(chunkData);

        response += " " + chunkHash;
        hash.push_back(chunkHash);       // Store hash in vector
        fileChunks.push_back(chunkData); // Store chunk data

        // cout << "Chunk " << (i / bufferSize) + 1 << " hash: " << chunkHash << endl;
    }

    // Close the file
    close(inputFile);
    delete[] buffer;

    // Compute the master SHA1 hash (hash of the entire file)
    string fullFileData;
    for (const auto &chunk : fileChunks)
    {
        fullFileData += chunk;
    }

    string masterHash = computeSHA1(fullFileData);
    hash.insert(hash.begin(), masterHash);
    response = masterHash + response;
    // cout << "Master SHA1 hash of the entire file: " << masterHash << endl;

    return response;
}