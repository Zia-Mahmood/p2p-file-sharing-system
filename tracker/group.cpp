#include "tracker.h"

// Method to serialize a User object to a char buffer
string Fileinfo::serialize() const
{
    stringstream ss;
    ss << filename << ",";

    // Serialize hashes
    for (const auto &hash : hashes)
    {
        ss << hash << ",";
    }

    ss << "$"; // Use a delimiter for clients_sharing
    for (const auto &client : clients_sharing)
    {
        ss << client << ",";
    }

    return ss.str();
}

// Static method to deserialize a char buffer into a User object
Fileinfo Fileinfo::deserialize(const string &line)
{
    Fileinfo fileinfo;
    stringstream ss(line);
    string token;

    getline(ss, fileinfo.filename, ',');

    // Deserialize hashes
    getline(ss, token, '$');
    stringstream hashesStream(token);
    while (getline(hashesStream, token, ','))
    {
        if (!token.empty())
        {
            fileinfo.hashes.push_back(token);
        }
    }

    // Deserialize clients_sharing
    getline(ss, token, '$');
    stringstream clientsStream(token);
    while (getline(clientsStream, token, ','))
    {
        if (!token.empty())
        {
            fileinfo.clients_sharing.push_back(token);
        }
    }

    return fileinfo;
}

// Method to serialize a Group object to a string
string Group::serialize() const
{
    stringstream ss;
    ss << group_id << ","
       << owner << ",";

    // Serialize members
    for (const auto &member : members)
    {
        ss << member << ",";
    }

    // Serialize pending requests
    ss << "|"; // Use a delimiter for pending requests
    for (const auto &request : pendingRequest)
    {
        ss << request << ",";
    }

    // Serialize files
    ss << "|"; // Use a delimiter for files
    for (const auto &file : files)
    {
        ss << file.first << ":" << file.second.serialize() << "|";
    }

    return ss.str();
}

// Static method to deserialize a string into a Group object
Group Group::deserialize(const string &line)
{
    Group group;
    stringstream ss(line);
    string token;

    getline(ss, group.group_id, ',');
    getline(ss, group.owner, ',');

    // Deserialize members
    getline(ss, token, '|'); // Read until the first delimiter
    stringstream membersStream(token);
    while (getline(membersStream, token, ','))
    {
        group.members.push_back(token);
    }

    // Deserialize pending requests
    getline(ss, token, '|'); // Read until the second delimiter
    stringstream requestsStream(token);
    while (getline(requestsStream, token, ','))
    {
        group.pendingRequest.push_back(token);
        // cout << "Pending Requests " << token << endl;
    }

    // Deserialize files
    while (getline(ss, token, '|'))
    {
        size_t pos = token.find(':');
        string fileKey = token.substr(0, pos);
        string fileValue = token.substr(pos + 1);
        // cout << "key: " << fileKey << " value: " << fileValue << endl;
        group.files[fileKey] = Fileinfo::deserialize(fileValue);
    }

    return group;
}

// Function to serialize a vector of Group objects into a single string
string serializeGroups()
{
    stringstream ss;
    for (const auto &[id, group] : groups)
    {
        ss << group.serialize() << " "; // Each group serialized on a new line
    }
    return ss.str();
}

// Function to deserialize a string into a vector of Group objects
void deserializeGroups(const string &data)
{
    // groups.clear();
    stringstream ss(data);
    string line;

    while (getline(ss, line, ' '))
    {
        if (!line.empty())
        { // Avoid processing empty lines
            Group new_group = Group::deserialize(line);
            groups[new_group.group_id] = new_group;
        }
    }
}
