#include "tracker.h"

// Serialization of data present with this Tracker
string serializeData()
{
    stringstream ss;
    ss << serializeUsers() << "| " << serializeGroups();
    return ss.str();
}

// Deserialization of data received by this Tracker, (synching)
int deserializeData(const string &response)
{
    if (response.find("sync_data ") != 0)
    {
        cout << "Invalid data format!" << endl;
        return 0;
    }
    string data = response.substr(10);
    stringstream ss(data);
    string segment;

    getline(ss, segment, '|');
    string userData = segment;
    deserializeUsers(userData);

    string groupData;
    getline(ss, groupData, ' ');
    getline(ss, groupData);
    deserializeGroups(groupData);
    return 1;
}
