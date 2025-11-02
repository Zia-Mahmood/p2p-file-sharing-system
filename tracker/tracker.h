#ifndef TRACKER_H
#define TRACKER_H

#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <sstream>
#include <atomic>
#include <algorithm>

using namespace std;

// Global definitions
#define MAX_TRACKERS 4
#define MAX_CONNECTIONS 20 // Max clients allowed to connect

// Tracker Information
struct TrackerInfo
{
    char ip[16];
    int port;
    bool is_online;
};

extern map<int, TrackerInfo> trackers;
extern int curTrackerId;
extern atomic<bool> isRunning;

// User Structure
struct User
{
    string username;
    string password;
    vector<string> groupsPartOf;
    char ip[16];
    int port;
    bool is_logged_in = false;
    map<string, string> shared_files;

    // Default constructor
    User();

    // Constructor to initialize the User object
    User(string user, string pass, vector<string> groups, const char *ipAddr, int p, bool loggedIn, map<string, string> files);

    // Method to serialize a User object to a char buffer
    string serialize() const;

    // Static method to deserialize a char buffer into a User object
    static User deserialize(const string &line);
};

// File structure
struct Fileinfo
{
    string filename;
    vector<string> hashes;
    vector<string> clients_sharing;

    // Method to serialize a User object to a char buffer
    string serialize() const;

    // Static method to deserialize a char buffer into a User object
    static Fileinfo deserialize(const string &line);
};

// Group structure
struct Group
{
    string group_id;
    string owner;
    vector<string> members;
    vector<string> pendingRequest;
    map<string, Fileinfo> files;

    string serialize() const;                     // Serialize Group object to string
    static Group deserialize(const string &line); // Deserialize string to Group object
};

// Maps to store users and groups
extern map<string, User> users;
extern map<string, Group> groups;

// Mutex for controlling access to users/groups in a multithreaded environment
extern mutex user_group_mutex;

// Function declarations

// Function to serialize a vector of User objects into a single string
string serializeUsers();

// Function to deserialize a string into a vector of User objects
void deserializeUsers(const string &data);

// Function to serialize a vector of Group objects into a single string
string serializeGroups();

// Function to deserialize a string into a vector of Group objects
void deserializeGroups(const string &data);

// Serialization of data present with this Tracker
string serializeData();

// Deserialization of data received by this Tracker, (synching)
int deserializeData(const string &response);

// Function to load tracker information from file using system calls
bool loadTrackerInfo(const char *filename, map<int, TrackerInfo> &trackers);

// Function to send I am online and sync the data
void sendOnlineNotification();

// Function to create socket through which tracker will listen to incoming connections
int createTrackerListeningSocket(const char *ip, int port);

// Signal handler to catch the termination signal and perform clean-up
void handleSignal();

// Function to monitor for 'exit' command
void monitorExitCommand();

// Function to send response of accepted/reject request of joining group
void sendResponse(string response, string username);

// Function to register a new user
string createUser(const string &username, const string &password, const char *ip, const int port);

// Function to return serialize Shared files
string getSharedFiles(const string &username);

// Function to login a user
string loginUser(const string &username, const string &password, const char *ip, const int port);

// Function to create a new group
string createGroup(const string &group_id, const string &owner);

// Function to handle join request to join a group
string joinGroup(const string &group_id, const string &username);

// Function to leave a group
string leaveGroup(const string &group_id, const string &username);

// Function to list pending join requests
string listRequests(const string &group_id, const string &username);

// Function to accept a pending group join request
string acceptRequest(const string &group_id, const string &user_id, const string &username);

// Function to reject a pending group join request
string rejectRequest(const string &group_id, const string &user_id, const string &username);

// Function to list all groups in the network
string listGroups();

// Function to list groups a particular user is part of
string listGroupsUserPartOf(const string &username);

// Function to log out a user
string logout(const string &username);

// Function to handle client requests
void handleClientRequest(int client_socket, struct sockaddr_in client_addr);

// Function to handle I_am_online
string trackerIsOnline(int id);

// Function to sync data received from trackers
string sync_data(string data);

// Function to handle when other trackers go offline
void trackerIsOffline(int id);

// Function to sync_data with all online trackers
void sync_data();

// Function to List All sharable Files In Group
string listFiles(const string &group_id, const string &username);

// Function to separate file name from file path
void separateFilePath(const string &fullPath, string &filePath, string &fileName);

// Function to Verify hashes
bool verifyHashes(string hash1, string hash2);

// Function to Upload File
string uploadFile(const string &file_full_path, const string &group_id, const string &username, const vector<string> &hashes);

// Function to serialize the download info to be sent to client
string serializeFile(Fileinfo &file);

// Function to Download File
string downloadFile(const string &group_id, const string &file_name, const string &username);

#endif // TRACKER_H
