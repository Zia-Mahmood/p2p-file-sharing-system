#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <map>
#include <mutex>
#include <algorithm>
#include <random>
#include <sstream>

#define MAX_TRACKERS 4
#define MAX_CONNECTIONS 20

using namespace std;

struct TrackerInfo
{
    char ip[16];
    int port;
    bool is_online;
};

struct User
{
    string username;
    string password;
    bool is_logged_in = false;
    map<string, string> shared_files;

    // Static method to deserialize a char buffer into a User previous shared files
    static void deserialize(const string &line);
};

// File structure
struct Fileinfo
{
    string file_path;
    string group_id;
    bool is_complete = false;
    bool is_download_file = true;
    bool is_sharing = false;
    string master_hash;
    vector<string> order_of_chunks;
    map<string, vector<string>> chunk_map;
    map<string, string> hashed_file;
};

extern map<int, TrackerInfo> trackers;
extern map<string, Fileinfo> files;
extern int curTrackerId;
extern int tracker_socket;
extern atomic<bool> isRunning;
extern User curr_user;
extern mutex client_mutex;
extern char client_ip[16];
extern int client_port;

// Function to load tracker information from file using system calls
bool loadTrackerInfo(const char *filename, map<int, TrackerInfo> &trackers);

// Function to connect to a tracker
int connectToTracker(int trackerid);

// Function to send a request to the tracker and get response
string sendRequest(int sock, const string &request);

// Function to validate if a user is logged in for commands 3-10
bool isLoggedIn();

// Function to create socket through which client will listen to incoming connections.
int createClientListeningSocket();

// Function to connect with new Tracker
bool reconnectToTracker(int trackerid);

// Function to prompt the user to share a file
bool promptShare(string filename);

// Function to handle client requests
void handleSlaveRequest(int client_socket, struct sockaddr_in client_addr);

// Function for listening request
void listenRequest();

// Handling create_user input
void createUser(char *data, string command);

// Handling login input
void login(char *data, string command);

// Handling logout input
void logout(char *data, string command);

// Handling create_group input
void createGroup(char *data, string command);

// Handling join_group input
void joinGroup(char *data, string command);

// Handling leave_group input
void leaveGroup(char *data, string command);

// Handling list_requests input
void listRequests(char *data, string command);

// Handling accept_request input
void acceptRequest(char *data, string command);

// Handling reject_request input
void rejectRequest(char *data, string command);

// Handling list_groups input
void listGroups(char *data, string command);

// Handling list_my_groups input
void listMyGroups(char *data, string command);

// Function to handle listing of files
void listFiles(char *data, string command);

// Function to compute the SHA1 hash of a given data chunk
string computeSHA1(const string &data);

// Function to send a particular chunk of file
string sendChunk(string file_full_path, string chunk_hash);

// Function to initialize hashes of the file and serialize them
string initializeHashes(string file_full_path, vector<string> &hash);

// Function to separate file name from file path
void separateFilePath(const string &fullPath, string &filePath, string &fileName);

// Handling upload_file input
void uploadFile(char *data, string command);

// Function to get available chunks info from all sharing clients
void getChunksAvailable(string peer, const string &master_hash, const string &file_name);

// Helper function to check if a directory exists
bool directoryExists(const string &dirPath);

// printing progress bar of percentage file written on shell
void printProgressBar(double progress);

// Function to construct the downloaded file.
bool constructFile(string file_name);

// Function to handle download input
void downloadFile(char *data, string command);

// Function to deserialize the response for download input
void deserialize(const string &response, vector<string> &hashes, vector<string> &peers);

// Function to show Downloads
void showDownloads();

// Function to stop sharing file in group
void stopShare(char *data, string command);

#endif // CLIENT_H