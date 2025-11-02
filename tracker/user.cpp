#include "tracker.h"

// Default constructor
User::User() : username(""), password(""), groupsPartOf(), port(0), is_logged_in(false)
{
    memset(ip, 0, sizeof(ip)); // Initialize the IP address to an empty string
}

// Constructor to initialize the User object
User::User(string user, string pass, vector<string> groups, const char *ipAddr, int p, bool loggedIn, map<string, string> files)
    : username(user), password(pass), groupsPartOf(groups), port(p), is_logged_in(loggedIn), shared_files(files)
{
    strncpy(ip, ipAddr, sizeof(ip)); // Copy IP address into the ip array
}

// Method to serialize a User object to a CSV line
string User::serialize() const
{
    stringstream ss;
    ss << username << ","
       << password << ","
       << string(ip) << ","
       << port << ","
       << boolalpha << is_logged_in;

    // Serialize groupsPartOf
    for (const auto &group : groupsPartOf)
    {
        ss << "," << group;
    }
    // Serialize shared_files
    ss << "$"; // Use a delimiter for shared_files
    for (const auto &file : shared_files)
    {
        ss << file.first << ":" << file.second << ",";
    }
    return ss.str();
}

// Static method to deserialize a CSV line into a User object
User User::deserialize(const string &line)
{
    User user("", "", {}, "", 0, false, {}); // Default initialization
    stringstream ss(line);
    string token;

    getline(ss, user.username, ',');
    getline(ss, user.password, ',');
    ss.get(user.ip, 16, ','); // Read the IP
    ss.ignore();              // Ignore the comma
    ss >> user.port;
    ss.ignore(); // Ignore the comma

    // Deserialize is_logged_in correctly
    string boolStr;
    getline(ss, boolStr, ',');
    // cout << "boolstr is " << boolStr << endl;                      // Get the boolean string
    user.is_logged_in = (boolStr == "true" || boolStr == "true$"); // Set boolean value

    // Deserialize groupsPartOf
    getline(ss, token, '$');
    stringstream groupsStream(token);
    while (getline(groupsStream, token, ','))
    {
        if (!token.empty())
        {
            user.groupsPartOf.push_back(token);
        }
    }

    // Deserialize shared_files
    while (getline(ss, token, ','))
    {
        if (!token.empty())
        {
            size_t pos = token.find(':');
            string fileKey = token.substr(0, pos);
            string fileValue = token.substr(pos + 1);
            user.shared_files[fileKey] = fileValue;
        }
    }

    return user;
}

// Function to serialize a vector of User objects into a single string
string serializeUsers()
{
    stringstream ss;
    for (const auto &[id, user] : users)
    {
        ss << user.serialize() << " "; // Each user serialized on a new line
    }
    return ss.str();
}

// Function to deserialize a string into a vector of User objects
void deserializeUsers(const string &data)
{
    // users.clear();
    stringstream ss(data);
    string line;

    while (getline(ss, line, ' '))
    {
        if (!line.empty())
        { // Avoid processing empty lines
            User new_user = User::deserialize(line);
            users[new_user.username] = new_user;
        }
    }
}
