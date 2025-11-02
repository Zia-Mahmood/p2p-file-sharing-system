#include "tracker.h"

map<int, TrackerInfo> trackers;
int curTrackerId;
atomic<bool> isRunning(true);

// Maps to store users and groups
map<string, User> users; // Maps username -> User object
map<string, Group> groups;

// Mutex to control access to users/groups (since it will be accessed by multiple threads)
mutex user_group_mutex;

// Function to load tracker information from file using system calls
bool loadTrackerInfo(const char *filename, map<int, TrackerInfo> &trackers)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        cout << "Error opening tracker_info.txt\n";
        return false;
    }

    char buffer[512];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0)
    {
        cout << "Error reading tracker_info.txt\n";
        close(fd);
        return false;
    }
    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    // Parse the tracker information
    char *line = strtok(buffer, "\n");
    int count = 0;
    while (line && count < MAX_TRACKERS)
    {
        TrackerInfo info;
        int id;
        sscanf(line, "%d %s %d", &id, info.ip, &info.port);
        info.is_online = false;
        trackers[id] = info;
        count++;
        line = strtok(NULL, "\n");
    }

    close(fd);
    return true;
}

// Function to send I am online and sync the data
void sendOnlineNotification()
{
    for (const auto &[id, tracker] : trackers)
    {
        if (id != curTrackerId)
        {

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                // cout << "Error creating socket for sending I_am_online.\n";
                continue;
            }

            struct sockaddr_in tracker_addr;
            tracker_addr.sin_family = AF_INET;
            tracker_addr.sin_port = htons(tracker.port);

            if (inet_pton(AF_INET, tracker.ip, &tracker_addr.sin_addr) <= 0)
            {
                // cout << "Invalid address for tracker: " << tracker.ip << endl;
                close(sock);
                continue;
            }

            if (connect(sock, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0)
            {
                // cout << "Error connecting to tracker: " << id << "\n";
                close(sock);
                continue;
            }

            trackers[id].is_online = true;

            string online_message = "I_am_online " + to_string(curTrackerId);
            // cout << "Sending online_message: " << online_message << endl;
            send(sock, online_message.c_str(), online_message.size(), 0);

            char buffer[20480] = {0};
            read(sock, buffer, 20480);
            // cout << "Data received for synchronization: " << buffer << endl;
            deserializeData(buffer);
            // deserialize and store the data
            close(sock);
        }
    }
    sync_data();
}

// Function to sync_data with all online trackers
void sync_data()
{
    for (const auto &[id, tracker] : trackers)
    {
        if (id != curTrackerId && trackers[id].is_online)
        {

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                // cout << "Error creating socket for sending sync_data.\n";
                continue;
            }

            struct sockaddr_in tracker_addr;
            tracker_addr.sin_family = AF_INET;
            tracker_addr.sin_port = htons(tracker.port);

            if (inet_pton(AF_INET, tracker.ip, &tracker_addr.sin_addr) <= 0)
            {
                // cout << "Invalid address for tracker: " << tracker.ip << endl;
                close(sock);
                continue;
            }

            if (connect(sock, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0)
            {
                // cout << "Error connecting to tracker: " << id << "\n";
                close(sock);
                continue;
            }

            string online_message = "sync_data " + serializeData(); // add serialization function todo
            send(sock, online_message.c_str(), online_message.size(), 0);

            char acknowledgement[100] = {0};
            read(sock, acknowledgement, 100);
            cout << "acknowledgement received: " << acknowledgement << endl;
            // deserialize and store the data todo
            close(sock);
        }
    }
}

// Function to create socket through which tracker will listen to incoming connections.
int createTrackerListeningSocket(const char *ip, int port)
{

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        cout << "Socket creation failed. Sorry Try Again!!\n";
        exit(0);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        // cout << "Setting sockopt failed\n";
    }

    // Assign IP and port to the socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    // Bind the socket to the IP and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cout << "Binding failed. Try again might work this time!!\n";
        close(server_fd);
        exit(0);
    }

    // Start listening for incoming connections
    if (listen(server_fd, MAX_CONNECTIONS) < 0)
    {
        cout << "Listen failed. We were very near for a P2P system. Try again!!\n";
        close(server_fd);
        exit(0);
    }

    cout << "Tracker is now listening on " << ip << ":" << port << endl;
    return server_fd;
}

// Function to handle client requests
void handleClientRequest(int client_socket, struct sockaddr_in client_addr)
{
    while (isRunning)
    {

        char buffer[20480] = {0};
        int readvalue = read(client_socket, buffer, 20480);
        if (readvalue < 1)
        {
            break;
        }
        cout << "msg received: " << buffer << endl;
        string request(buffer);
        string response;

        // Split the request into command and arguments
        char *token = strtok(buffer, " ");
        // istringstream iss(request);
        string command;
        // iss >> command;
        // cout << "TOken " << token << endl;
        if (strcmp(token, "create_user") == 0)
        {
            string username, password, ip, port;
            // iss >> username >> password;
            token = strtok(NULL, " ");
            username = token;
            token = strtok(NULL, " ");
            password = token;
            token = strtok(NULL, " ");
            ip = token;
            token = strtok(NULL, " ");
            port = token;
            response = createUser(username, password, ip.c_str(), stoi(port));
        }
        else if (strcmp(token, "login") == 0)
        {
            string username, password, ip, port;
            // iss >> username >> password;
            token = strtok(NULL, " ");
            username = token;
            token = strtok(NULL, " ");
            password = token;
            token = strtok(NULL, " ");
            ip = token;
            token = strtok(NULL, " ");
            port = token;
            response = loginUser(username, password, ip.c_str(), stoi(port));
        }
        else if (strcmp(token, "create_group") == 0)
        {
            string group_id, owner;
            // iss >> group_id >> owner;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            owner = token;
            response = createGroup(group_id, owner);
        }
        else if (strcmp(token, "join_group") == 0)
        {
            string group_id, username;
            // iss >> group_id >> username;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            username = token;
            response = joinGroup(group_id, username);
        }
        else if (strcmp(token, "leave_group") == 0)
        {
            string group_id, username;
            // iss >> group_id >> username;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            username = token;
            response = leaveGroup(group_id, username);
        }
        else if (strcmp(token, "list_requests") == 0)
        {
            string group_id, username;
            // iss >> group_id >> username;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            username = token;
            response = listRequests(group_id, username);
        }
        else if (strcmp(token, "accept_request") == 0)
        {
            string group_id, user_id, username;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            user_id = token;
            token = strtok(NULL, " ");
            username = token;
            response = acceptRequest(group_id, user_id, username);
        }
        else if (strcmp(token, "reject_request") == 0)
        {
            string group_id, user_id, username;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            user_id = token;
            token = strtok(NULL, " ");
            username = token;
            response = rejectRequest(group_id, user_id, username);
        }
        else if (strcmp(token, "list_groups") == 0)
        {
            response = listGroups();
        }
        else if (strcmp(token, "list_my_groups") == 0)
        {
            string username;
            token = strtok(NULL, " ");
            username = token;
            response = listGroupsUserPartOf(username);
        }
        else if (strcmp(token, "logout") == 0)
        {
            string username;
            token = strtok(NULL, " ");
            username = token;
            response = logout(username);
        }
        else if (strcmp(token, "list_files") == 0)
        {
            string group_id, username;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            username = token;
            response = listFiles(group_id, username);
        }
        else if (strcmp(token, "upload_file") == 0)
        {
            string file_full_path, group_id, username;
            vector<string> hashes;
            token = strtok(NULL, " ");
            file_full_path = token;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            username = token;
            token = strtok(NULL, " ");
            while (token != NULL)
            {
                hashes.push_back(string(token));
                token = strtok(NULL, " ");
            }
            response = uploadFile(file_full_path, group_id, username, hashes);
        }
        else if (strcmp(token, "download_file") == 0)
        {
            string group_id, file_name, file_path, username;
            token = strtok(NULL, " ");
            group_id = token;
            token = strtok(NULL, " ");
            file_name = token;
            token = strtok(NULL, " ");
            username = token;
            token = strtok(NULL, " ");

            response = downloadFile(group_id, file_name, username);
        }
        else if (strcmp(token, "exit") == 0)
        {
            string username;
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                break;
            }
            username = token;
            response = logout(username);
            sync_data();
            break;
        }
        else if (strcmp(token, "I_am_online") == 0)
        {
            string tracker_id;
            token = strtok(NULL, " ");
            tracker_id = token;
            response = trackerIsOnline(stoi(tracker_id));
            send(client_socket, response.c_str(), response.size(), 0);
            break;
        }
        else if (strcmp(token, "sync_data") == 0)
        {
            string data;
            token = strtok(NULL, "\0");
            data = token;
            response = sync_data("sync_data " + data);
            send(client_socket, response.c_str(), response.size(), 0);
            break;
        }
        else if (strcmp(token, "OFFLINE") == 0)
        {
            string tracker_id;
            token = strtok(NULL, " ");
            tracker_id = token;
            trackerIsOffline(stoi(tracker_id));
            break;
        }
        else
        {
            response = "INVALID COMMAND";
        }
        send(client_socket, response.c_str(), response.size(), 0);
        sync_data();
    }
    close(client_socket);
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <tracker_info.txt> <tracker_id>\n";
        return 1;
    }

    const char *trackerFile = argv[1];
    curTrackerId = atoi(argv[2]);

    if (!loadTrackerInfo(trackerFile, trackers))
    {
        cout << "unable to load tracker info from torrent file" << endl;
        return 1;
    }

    bool found = false;
    for (auto &[id, tracker] : trackers)
    {
        if (id == curTrackerId)
        {
            // todo
            sendOnlineNotification();

            //  Create and bind the socket for the current tracker
            int server_listening_fd = createTrackerListeningSocket(tracker.ip, tracker.port);
            trackers[id].is_online = true;

            // Start a separate thread to monitor the 'exit' command
            thread exit_thread(monitorExitCommand);

            cout << "Tracker is running and listening for connections...\n";

            while (isRunning)
            {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int new_socket = accept(server_listening_fd, (struct sockaddr *)&client_addr, &addr_len);
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                int client_port = ntohs(client_addr.sin_port);
                cout << "Client connected: " << client_ip << ":" << client_port << endl;
                if (new_socket >= 0)
                {
                    // Create a new thread for handling client requests
                    thread client_thread(handleClientRequest, new_socket, client_addr);
                    client_thread.detach(); // Detach the thread
                }
            }
            found = true;
            close(server_listening_fd);
            if (exit_thread.joinable())
            {
                exit_thread.join();
            }

            cout << "Tracker has shut down." << endl;
            break;
        }
    }

    if (!found)
    {
        cout << "Tracker with ID " << curTrackerId << " not found in " << trackerFile << endl;
        return 1;
    }
    return 0;
}