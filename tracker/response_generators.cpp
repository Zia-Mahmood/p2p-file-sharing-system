#include "tracker.h"

// Function to register a new user
string createUser(const string &username, const string &password, const char *ip, const int port)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (users.find(username) == users.end())
    {
        User newUser = {username, password, {}, ip, port, false, {}};
        users[username] = newUser;
        response = "user_created_successfully";
        cout << "User " << username << " created successfully." << endl;
    }
    else
    {
        response = "user_already_exists";
        cout << "User " << username << " already exists!" << endl;
    }
    return response;
}

// Function to return serialize Shared files
string getSharedFiles(const string &username)
{
    stringstream ss;
    for (const auto &file : users[username].shared_files)
    {
        ss << file.first << ":" << file.second << " ";
        for (const auto &[id, group] : groups)
        {
            if (group.files.find(file.first) != group.files.end())
            {
                ss << id << ",";
                break;
            }
        }
    }
    return ss.str();
}

// Function to login a user
string loginUser(const string &username, const string &password, const char *ip, const int port)
{
    lock_guard<mutex> lock(user_group_mutex);
    string respone;
    if (users.find(username) != users.end() && users[username].password == password)
    {
        if (!users[username].is_logged_in)
        {
            users[username].is_logged_in = true;
            strcpy(users[username].ip, ip);
            users[username].port = port;
            respone = getSharedFiles(username);
            if (respone == "")
            {
                respone = " ";
            }
            cout << "User " << username << " logged in successfully. " << endl;
        }
        else
        {
            respone = "user_is_already_logged_in";
            cout << "User " << username << " is already logged in." << endl;
        }
    }
    else
    {
        respone = "invalid_username_password";
        cout << "Invalid username or password." << endl;
    }
    return respone;
}

// Function to create a new group
string createGroup(const string &group_id, const string &owner)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (groups.find(group_id) == groups.end())
    {
        Group newGroup = {group_id, owner, {owner}, {}, {}};
        groups[group_id] = newGroup;
        users[owner].groupsPartOf.push_back(group_id);
        response = "group_created_successfully";
        cout << "Group " << group_id << " created successfully." << endl;
    }
    else
    {
        response = "group_already_exists";
        cout << "Group " << group_id << " already exists!" << endl;
    }
    return response;
}

// Function to handle join request to join a group
string joinGroup(const string &group_id, const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (groups.find(group_id) != groups.end())
    {
        vector<string> member = groups[group_id].members;
        vector<string> pending = groups[group_id].pendingRequest;
        if (find(member.begin(), member.end(), username) == member.end())
        {
            if (find(pending.begin(), pending.end(), username) == pending.end())
            {
                groups[group_id].pendingRequest.push_back(username);
                response = "user_requested_to_join_group";
                cout << "User " << username << " requested to join group " << group_id << " successfully." << endl;
            }
            else
            {
                response = "join_request_already_exist";
                cout << "User " << username << " has already requested for joining group " << group_id << ".\n";
            }
        }
        else
        {
            response = "user_already_member_of_group";
            cout << "User " << username << " is already a member of group " << group_id << "." << endl;
        }
    }
    else
    {
        response = "group_not_exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}

// Function to leave a group
string leaveGroup(const string &group_id, const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (groups.find(group_id) != groups.end())
    {
        auto &member = groups[group_id].members;
        auto it = find(member.begin(), member.end(), username);
        if (it != member.end())
        {
            member.erase(it);
            auto itr = find(users[username].groupsPartOf.begin(), users[username].groupsPartOf.end(), group_id);
            users[username].groupsPartOf.erase(itr);
            if (groups[group_id].owner == username)
            {
                if (member.size() > 0)
                {
                    groups[group_id].owner = member[0];
                }
                else
                {
                    groups.erase(group_id);
                }
            }
            response = "user_left_group_successfully";
            cout << "User " << username << " left group " << group_id << " successfully." << endl;
        }
        else
        {
            response = "user_not_member_of_group";
            cout << "User " << username << " is not a member of group " << group_id << "." << endl;
        }
    }
    else
    {
        response = "group_not_exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}

// Function to list pending join requests
string listRequests(const string &group_id, const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response = " ";
    if (groups.find(group_id) != groups.end())
    {
        if (groups[group_id].owner == username)
        {
            vector<string> pending = groups[group_id].pendingRequest;
            response = " ";
            cout << "Pending request for group " << group_id << ".\n";
            for (string user : pending)
            {
                response += user + " ";
                cout << user << " ";
            }
            cout << endl;
        }
        else
        {
            response = "user_not_authorized_to_see_pending_request_for_this_group";
            cout << "User " << username << " is not authorized to see pending requests of group " << group_id << " because he is not the owner of this group." << endl;
        }
    }
    else
    {
        response = "group_not_exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}

// Function to accept a pending group join request
string acceptRequest(const string &group_id, const string &user_id, const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (groups.find(group_id) != groups.end())
    {
        if (groups[group_id].owner == username)
        {
            auto &pending = groups[group_id].pendingRequest;
            auto &member = groups[group_id].members;
            if (find(pending.begin(), pending.end(), user_id) != pending.end())
            {
                response = "user_added_successfully";
                sendResponse(response + " to group " + group_id, user_id);
                groups[group_id].members.push_back(user_id);
                users[user_id].groupsPartOf.push_back(group_id);
                pending.erase(find(pending.begin(), pending.end(), user_id));
                cout << "User " << user_id << " has been added to group " << group_id << "." << endl;
            }
            else if (find(member.begin(), member.end(), user_id) != member.end())
            {
                response = "user_already_member_of_group";
                cout << "User " << user_id << " is already member of group " << group_id << ".\n";
            }
            else
            {
                response = "user_not_requested_to_join_group";
                cout << "User " << user_id << " did not requested to join group" << group_id << ".\n";
            }
        }
        else
        {
            response = "user_not_authorized_to_accept_members_for_this_group";
            cout << "User " << username << " is not authorized to accept members of group " << group_id << " because he is not the owner of this group." << endl;
        }
    }
    else
    {
        response = "group_not exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}

// Function to send response of accepted/reject request of joining group
void sendResponse(string response, string username)
{
    User user = users[username];
    int count = 1;
    while (user.is_logged_in && count <= 3)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            // cout << "Failed to create socket to notify user." << endl;
            // cout << "Failed to tell User: " << id << " Attempt " << count << " failed." << endl;
            count++;
            continue;
        }

        struct sockaddr_in user_addr;
        user_addr.sin_family = AF_INET;
        user_addr.sin_port = htons(user.port);

        if (inet_pton(AF_INET, user.ip, &user_addr.sin_addr) <= 0)
        {
            cout << "Invalid address for user: " << user.ip << endl;
            close(sock);
            break;
        }

        if (connect(sock, (struct sockaddr *)&user_addr, sizeof(user_addr)) >= 0)
        {
            send(sock, response.c_str(), response.size(), 0);
            close(sock);
            break;
        }
        // cout << "Failed to connect to user " << user.ip << ":" << user.port << " Attempt " << count << " failed." << endl;
        count++;
        close(sock);
    }
}

// Function to reject a pending group join request
string rejectRequest(const string &group_id, const string &user_id, const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (groups.find(group_id) != groups.end())
    {
        if (groups[group_id].owner == username)
        {
            auto &pending = groups[group_id].pendingRequest;
            auto &member = groups[group_id].members;
            if (find(pending.begin(), pending.end(), user_id) != pending.end())
            {
                response = "user_rejected_successfully";
                sendResponse(response + " from joining group " + group_id, user_id);
                pending.erase(find(pending.begin(), pending.end(), user_id));
                cout << "User " << user_id << " request to join has been rejected by the owner of group " << group_id << "." << endl;
            }
            else if (find(member.begin(), member.end(), user_id) != member.end())
            {
                response = "user_already_member_of_group";
                cout << "User " << user_id << " is already member of group " << group_id << ".\n";
            }
            else
            {
                response = "user_not_requested_to_join_group";
                cout << "User " << user_id << " did not requested to join group" << group_id << ".\n";
            }
        }
        else
        {
            response = "user_not_authorized_to_accept_members_for_this_group";
            cout << "User " << username << " is not authorized to accept members of group " << group_id << " because he is not the owner of this group." << endl;
        }
    }
    else
    {
        response = "group_not exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}

// Function to list all groups in the network
string listGroups()
{
    lock_guard<mutex> lock(user_group_mutex);
    string response = " ";
    cout << "Listing all groups in the network:" << endl;
    for (const auto &group : groups)
    {
        response += group.first + " ";
        cout << "Group ID: " << group.first << " ";
    }
    cout << endl;
    return response;
}

// Function to list groups particular user is part of
string listGroupsUserPartOf(const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response = " ";
    cout << "Listing all groups where user " << username << " is part of, in the network : " << endl;
    for (string groupd_id : users[username].groupsPartOf)
    {
        response += groupd_id + " ";
        cout << "Group ID: " << groupd_id << " ";
    }
    cout << endl;
    return response;
}

// Function to log out a user
string logout(const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (users.find(username) != users.end() && users[username].is_logged_in)
    {
        users[username].is_logged_in = false;
        cout << "User " << username << " logged out successfully." << endl;
        response = "user_logged_out";
    }
    else
    {
        cout << "User " << username << " is not logged in." << endl;
        response = "user_is_not_logged_in";
    }
    return response;
}

// Function to handle I_am_online
string trackerIsOnline(int id)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response = " ";
    if (trackers.find(id) != trackers.end())
    {
        trackers[id].is_online = true;
        response = "sync_data " + serializeData();
    }

    return response;
}

// Function to sync data received from trackers
string sync_data(string data)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response;
    if (deserializeData(data))
    {
        response = "OK";
    }
    else
    {
        response = "NOT_SYNCED";
    }
    return response;
}

// Function to List All sharable Files In Group
string listFiles(const string &group_id, const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response = " ";
    if (groups.find(group_id) != groups.end())
    {
        const Group &group = groups[group_id];
        if (find(group.members.begin(), group.members.end(), username) != group.members.end())
        {
            map<string, Fileinfo> files = group.files;
            response = " ";
            cout << "Files list of all the shareable files in the group" << group_id << ".\n";
            for (const auto &[filename, file] : files)
            {
                response += filename + " ";
                cout << filename << " ";
            }
            cout << endl;
        }
        else
        {
            response = "user_not_member_of_group";
            cout << "User " << username << "is not member of the group " << group_id << endl;
        }
    }
    else
    {
        response = "group_not_exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}

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

// Function to Verify hashes
bool verifyHashes(string hash1, string hash2)
{
    return hash1 == hash2;
}

// Function to Upload File
string uploadFile(const string &file_full_path, const string &group_id, const string &username, const vector<string> &hashes)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response = " ";
    if (groups.find(group_id) != groups.end())
    {
        const Group &group = groups[group_id];
        if (find(group.members.begin(), group.members.end(), username) != group.members.end())
        {
            map<string, Fileinfo> &files = groups[group_id].files;
            string file_path, file_name;
            separateFilePath(file_full_path, file_path, file_name);
            if (files.find(file_name) != files.end())
            { // file already present in group
                if (verifyHashes(hashes[0], files[file_name].hashes[0]))
                { // verifying whether same file is shared.
                    vector<string> &clients_sharing = files[file_name].clients_sharing;
                    if (find(clients_sharing.begin(), clients_sharing.end(), username) == clients_sharing.end())
                    {
                        clients_sharing.push_back(username);
                        users[username].shared_files[file_name] = file_path;
                        response = "file_uploaded_successfully";
                        cout << "File: " << file_name << " is now being shared by " << username << " in group " << group_id << endl;
                    }
                    else
                    {
                        response = "user_already_sharing_this_file";
                        cout << "User " << username << " is already sharing file " << file_name << " in the group " << group_id << endl;
                    }
                }
                else
                {
                    response = "incorrect_file_uploaded";
                    cout << "Hashes of file, already present and current file doesn't match" << endl;
                }
            }
            else
            {
                Fileinfo newFile = {file_name, hashes, {username}};
                files[file_name] = newFile;
                users[username].shared_files[file_name] = file_path;
                response = "file_uploaded_successfully";
                cout << "File: " << file_name << " is now being shared by " << username << " in group " << group_id << endl;
            }
        }
        else
        {
            response = "user_not_member_of_group";
            cout << "User " << username << "is not member of the group " << group_id << endl;
        }
    }
    else
    {
        response = "group_not_exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}

// Function to serialize the download info to be sent to client
string serializeFile(Fileinfo &file)
{
    stringstream ss;

    // Serialize hashes
    for (const auto &hash : file.hashes)
    {
        ss << hash << ",";
    }

    ss << "$"; // Use a delimiter for clients_sharing
    for (const auto &client : file.clients_sharing)
    {
        ss << users[client].ip << ":" << users[client].port << ",";
    }

    return ss.str();
}

// Function to Download File
string downloadFile(const string &group_id, const string &file_name, const string &username)
{
    lock_guard<mutex> lock(user_group_mutex);
    string response = " ";
    if (groups.find(group_id) != groups.end())
    {
        const Group &group = groups[group_id];
        if (find(group.members.begin(), group.members.end(), username) != group.members.end())
        {
            map<string, Fileinfo> &files = groups[group_id].files;
            if (files.find(file_name) != files.end())
            { // file already present in group
                response = serializeFile(files[file_name]);
                cout << "download info shared to user " << username << " for file " << file_name << " of group " << group_id << endl;
            }
            else
            {
                response = "file_not_present_for_sharing";
                cout << "File " << file_name << " is not present in the group " << group_id << endl;
            }
        }
        else
        {
            response = "user_not_member_of_group";
            cout << "User " << username << "is not member of the group " << group_id << endl;
        }
    }
    else
    {
        response = "group_not_exist";
        cout << "Group " << group_id << " does not exist." << endl;
    }
    return response;
}