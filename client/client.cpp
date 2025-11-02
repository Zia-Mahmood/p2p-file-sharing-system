#include "client.h"

map<int, TrackerInfo> trackers;
map<string, Fileinfo> files;
int curTrackerId;
int tracker_socket;
atomic<bool> isRunning(true);
User curr_user;
mutex client_mutex;
char client_ip[16];
int client_port;

// Static method to deserialize a CSV line into a User object
void User::deserialize(const string &line)
{
    stringstream ss(line);
    string token;

    // Deserialize shared_files
    while (getline(ss, token, ','))
    {
        if (!token.empty())
        {
            char *tok = strtok((char *)token.c_str(), ":");
            string fileKey = tok;
            tok = strtok(NULL, " ");
            string fileValue = tok;
            tok = strtok(NULL, "\0");
            string group_id;
            if (tok == NULL)
            {
                group_id = "";
            }
            else
            {
                group_id = tok;
            }
            // size_t pos = token.find(':');
            // string fileKey = token.substr(0, pos);
            // string fileValue = token.substr(pos + 1);
            curr_user.shared_files[fileKey] = fileValue;
            string file_full_path = fileValue + "/" + fileKey;
            int inputFile = open(file_full_path.c_str(), O_RDONLY);
            bool is_complete = true;
            if (inputFile == -1)
            {
                is_complete = false;
            }
            Fileinfo new_file = {fileValue, group_id, is_complete, false, true, "", {}, {}, {}};
            if (files.find(fileKey) != files.end() && files[fileKey].is_download_file)
            {
                new_file = files[fileKey];
            }
            if (is_complete)
            {
                vector<string> hash;
                string hashes = initializeHashes(file_full_path, hash);
                new_file.master_hash = hash[0];
                vector<string> ofc(hash.begin() + 1, hash.end());
                new_file.order_of_chunks = ofc;
                for (string sha : new_file.order_of_chunks)
                {
                    new_file.chunk_map[sha] = {};
                    new_file.hashed_file[sha] = "";
                }
                files[fileKey] = new_file;
            }
        }
    }
}

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

// Function to connect to a tracker
int connectToTracker(int trackerid)
{
    vector<int> arr = {1, 2, 3, 4};

    // Obtain a random seed from the random device
    random_device rd;
    mt19937 g(rd());
    shuffle(arr.begin(), arr.end(), g);
    int sock = -1;
    for (int id : arr)
    {
        if (id == trackerid)
        {
            continue;
        }
        int count = 1, flag = 0;
        while (count <= 3)
        {
            TrackerInfo tracker = trackers[id];
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                // cout << "Error in creating socket" << endl;
                cout << "Failed to connect to tracker: " << id << " Attempt " << count << " failed." << endl;
                count++;
                continue;
            }

            struct sockaddr_in tracker_addr;
            tracker_addr.sin_family = AF_INET;
            tracker_addr.sin_port = htons(tracker.port);

            if (inet_pton(AF_INET, tracker.ip, &tracker_addr.sin_addr) <= 0)
            {
                // cout << "Invalid address for tracker: " << tracker.ip << endl;
                cout << "Failed to connect to tracker: " << id << " Attempt " << count << " failed." << endl;
                count++;
                close(sock);
                continue;
            }

            if (connect(sock, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0)
            {
                // cout << "Connection to tracker " << tracker.ip << ":" << tracker.port << " failed." << endl;
                cout << "Failed to connect to tracker: " << id << " Attempt " << count << " failed." << endl;
                count++;
                close(sock);
            }
            else
            {
                cout << "Connected to tracker " << tracker.ip << ":" << tracker.port << endl;
                flag = 1;
                break;
            }
        }
        if (flag)
        {
            break;
        }
    }
    return sock;
}

// Function to send a request to the tracker and get response
string sendRequest(int sock, const string &request)
{

    // // char buffer[524290] = {0};
    // long unsigned int sent = send(sock, request.c_str(), request.size(), 0);
    // cout << "Total bytes sent: " << request.size() << " " << sent << endl;
    // const int CHUNK_SIZE = 524300; // 512 KB
    // char buffer[CHUNK_SIZE] = {0};
    // long unsigned int total_bytes_received = read(sock, buffer, CHUNK_SIZE);

    // cout << "Total bytes received: " << total_bytes_received << endl;

    // // Return the string containing the received data
    // return string(buffer, total_bytes_received);

    // Send the request to the server
    send(sock, request.c_str(), request.size(), 0);
    // cout << "Total bytes sent: " << request.size() << " " << sent << endl;

    const int CHUNK_SIZE = 65536; // 64 KB chunks for reading
    char buffer[CHUNK_SIZE] = {0};
    string total_data; // String to hold all received data

    long unsigned int total_bytes_received = 0;
    long unsigned int bytes_received = 0;

    // Keep reading data until all is received
    while (true)
    {
        // Read data from the socket in chunks of 64 KB
        bytes_received = read(sock, buffer, CHUNK_SIZE);

        if (bytes_received < 0)
        {
            // Error occurred, handle it
            cout << "Error in reading from socket" << endl;
            break;
        }
        else if (bytes_received == 0)
        {
            // No more data to read, exit the loop
            // cout << "No more data to receive (socket closed by peer)" << endl;
            break;
        }

        // Accumulate the received data
        total_data.append(buffer, bytes_received);
        total_bytes_received += bytes_received;

        // cout << "Bytes received in this iteration: " << bytes_received << " " << total_bytes_received << endl;

        // If we receive exactly CHUNK_SIZE (64 KB), there might be more data in the TCP window

        if (total_bytes_received == 8 * CHUNK_SIZE)
        {
            break;
        }

        if (bytes_received < CHUNK_SIZE)
        {
            // Less than CHUNK_SIZE indicates the last part of the data
            break;
        }
    }

    if (total_bytes_received > 5000)
    {
        cout << "Total bytes received: " << total_bytes_received << endl;
    }
    // Return the received data as a string
    return total_data;
}

// Function to validate if a user is logged in for commands 3-10
bool isLoggedIn()
{
    if (!curr_user.is_logged_in)
    {
        cout << "You need to login first." << endl;
        return false;
    }
    return true;
}

// Function to create socket through which client will listen to incoming connections.
int createClientListeningSocket()
{

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int buffer_size = 524300; // 512 KB
    setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    if (server_fd < 0)
    {
        cout << "Socket creation failed. Sorry Try Again!!\n";
        exit(0);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        cout << "Setting sockopt failed\n";
    }

    // Assign IP and port to the socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(client_ip);
    address.sin_port = htons(client_port);

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

    // cout << "Client is now listening on " << client_ip << ":" << client_port << endl;
    return server_fd;
}

// Function to connect with new Tracker
bool reconnectToTracker(int trackerid)
{
    lock_guard<mutex> lock(client_mutex);
    tracker_socket = connectToTracker(trackerid);
    if (tracker_socket < 0)
    {
        cout << "Failed to connect to any tracker." << endl;
    }
    return tracker_socket < 0;
}

// Function to prompt the user to share a file
bool promptShare(string filename)
{
    lock_guard<mutex> lock(client_mutex);
    cout << "A user wants to download a chunk of file " << filename << " from you" << endl;
    cout << "Allow user YES/NO? due to multithreading and single terminal output. set to auto yes on prompt?" << endl;
    string input = "YES";
    // cin >> input;
    if (input == "YES")
    {
        cout << "Sharing restarted." << endl;
        files[filename].is_sharing = true;
        return true;
    }
    cout << "I want to choke the client." << endl;
    files[filename].is_sharing = false;
    return false;
    // We can add user grant here and wait for the user to grant the access.
}

// Function to handle client requests
void handleSlaveRequest(int client_socket, struct sockaddr_in client_addr)
{
    while (isRunning)
    {

        char buffer[20480] = {0};
        int readvalue = read(client_socket, buffer, 20480);
        if (readvalue < 1)
        {
            break;
        }
        cout << "msg: " << buffer << endl;
        string request(buffer);
        string response;

        // Split the request into command and arguments
        char *token = strtok(buffer, " ");
        // istringstream iss(request);
        string command;
        // iss >> command;
        // cout << "TOken " << token << endl;
        if (strcmp(token, "shutting_down") == 0)
        {
            token = strtok(NULL, "\0");
            int trackerid = stoi(token);
            if (reconnectToTracker(trackerid))
            {
                isRunning = false;
                break;
            }
        }
        else if (strcmp(token, "have_file") == 0)
        {
            token = strtok(NULL, " ");
            string master_hash = token;
            string filename;
            bool ispresent = false;
            string response;
            if (isLoggedIn())
            {
                for (auto &[file_name, file] : files)
                {
                    if (file.master_hash == master_hash && file.is_sharing)
                    {
                        filename = file_name;
                        ispresent = true;
                        if (file.is_complete)
                        {
                            response = "yes_complete_file_available";
                        }
                        else
                        {
                            for (size_t i = 0; i < file.order_of_chunks.size(); i++)
                            {
                                if (file.hashed_file[file.order_of_chunks[i]].empty())
                                {
                                    continue;
                                }
                                else
                                {
                                    response += to_string(i) + " ";
                                }
                            }
                        }
                        break;
                    }
                }
                if (!ispresent)
                {
                    response = "file_not_available";
                }
                cout << "received request for chunks present at my end for file: " << filename << endl;
            }
            else
            {
                response = "user_is_not_logged_in";
            }
            send(client_socket, response.c_str(), response.size(), 0);
        }
        else if (strcmp(token, "send_chunk") == 0)
        {
            token = strtok(NULL, " ");
            string master_hash = token;
            token = strtok(NULL, " ");
            string chunk_hash = token;
            token = strtok(NULL, " ");
            string filename = token;
            string response;
            if (isLoggedIn())
            {
                if (files.find(filename) != files.end())
                {
                    if (files[filename].is_sharing)
                    {
                        if (files[filename].is_complete)
                        {

                            response = sendChunk(files[filename].file_path + "/" + filename, chunk_hash);
                        }
                        else
                        {
                            if (files[filename].hashed_file.find(chunk_hash) != files[filename].hashed_file.end() && !files[filename].hashed_file[chunk_hash].empty())
                            {
                                response = files[filename].hashed_file[chunk_hash];
                            }
                            else if (files[filename].hashed_file.find(chunk_hash) != files[filename].hashed_file.end())
                            {
                                response = "currently_this_chunk_is_not_available";
                            }
                            else
                            {
                                response = "incorrect_hash";
                            }
                        }
                    }
                    else
                    {
                        if (!promptShare(filename))
                        {
                            response = "user_not_sharing_currently";
                        }
                        else
                        {
                            if (files[filename].is_complete)
                            {
                                response = sendChunk(files[filename].file_path + "/" + filename, chunk_hash);
                            }
                            else
                            {
                                if (files[filename].hashed_file.find(chunk_hash) != files[filename].hashed_file.end() && !files[filename].hashed_file[chunk_hash].empty())
                                {
                                    response = files[filename].hashed_file[chunk_hash];
                                }
                                else if (files[filename].hashed_file.find(chunk_hash) != files[filename].hashed_file.end())
                                {
                                    response = "currently_this_chunk_is_not_available";
                                }
                                else
                                {
                                    response = "incorrect_hash";
                                }
                            }
                        }
                    }
                }
                else
                {
                    response = "file_not_available";
                }
            }
            else
            {
                response = "user_is_not_logged_in";
            }
            if (response.size() < 5000)
            {
                cout << "response for send chunk is " << response << endl;
            }
            else
            {
                cout << "Sending chunks of hash value: " << computeSHA1(response) << " and size " << response.size() << " bytes" << endl;
            }
            send(client_socket, response.c_str(), response.size(), 0);
        }
        else
        {
            cout << "Received a message from Tracker: " << request << endl;
        }
    }
}

// Function for listening request
void listenRequest()
{
    int client_listening_fd = createClientListeningSocket();
    // cout << "Client is running and listening for connections...\n";

    while (isRunning)
    {
        struct sockaddr_in slave_addr;
        socklen_t addr_len = sizeof(slave_addr);
        int new_socket = accept(client_listening_fd, (struct sockaddr *)&slave_addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(slave_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(slave_addr.sin_port);
        cout << "Peer connected: " << client_ip << ":" << client_port << endl;
        if (new_socket >= 0)
        {
            // Create a new thread for handling client requests
            thread slave_thread(handleSlaveRequest, new_socket, slave_addr);
            slave_thread.detach(); // Detach the thread
        }
    }
    cout << "Client has shut down." << endl;
    exit(1);
}

// Main function to handle client commands
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <IP>:<PORT> <tracker_info.txt>" << endl;
        return 1;
    }

    const char *trackerFile = argv[2];
    char *token = strtok(argv[1], ":");
    strcpy(client_ip, token);
    token = strtok(NULL, "\0");
    client_port = stoi(string(token));
    // cout << "ip is: " << client_ip << " port: " << client_port << endl;
    if (!loadTrackerInfo(trackerFile, trackers))
    {
        return 1;
    }

    // Connect to the tracker
    tracker_socket = connectToTracker(-1);
    if (tracker_socket < 0)
    {
        cout << "Failed to connect to any tracker." << endl;
        return 1;
    }
    thread recv_thread(listenRequest);
    recv_thread.detach();
    string input;

    while (true)
    {
        // this_thread::sleep_for(chrono::seconds(1));
        cout << "Enter command: ";
        getline(cin, input);
        string command = input;
        char *token = strtok((char *)input.c_str(), " ");
        // Parse and execute commands
        if (strcmp(token, "create_user") == 0)
        {
            token = strtok(NULL, "\0");
            createUser(token, command);
        }
        else if (strcmp(token, "login") == 0)
        {
            token = strtok(NULL, "\0");
            login(token, command);
        }
        else if (strcmp(token, "logout") == 0)
        {
            token = strtok(NULL, "\0");
            logout(token, command);
        }
        else if (strcmp(token, "create_group") == 0)
        {
            token = strtok(NULL, "\0");
            createGroup(token, command);
        }
        else if (strcmp(token, "join_group") == 0)
        {
            token = strtok(NULL, "\0");
            joinGroup(token, command);
        }
        else if (strcmp(token, "leave_group") == 0)
        {
            token = strtok(NULL, "\0");
            leaveGroup(token, command);
        }
        else if (strcmp(token, "list_requests") == 0)
        {
            token = strtok(NULL, "\0");
            listRequests(token, command);
        }
        else if (strcmp(token, "accept_request") == 0)
        {
            token = strtok(NULL, "\0");
            acceptRequest(token, command);
        }
        else if (strcmp(token, "reject_request") == 0)
        {
            token = strtok(NULL, "\0");
            rejectRequest(token, command);
        }
        else if (strcmp(token, "list_groups") == 0)
        {
            token = strtok(NULL, "\0");
            listGroups(token, command);
        }
        else if (strcmp(token, "list_my_groups") == 0)
        {
            token = strtok(NULL, "\0");
            listMyGroups(token, command);
        }
        else if (strcmp(token, "list_files") == 0)
        {
            token = strtok(NULL, "\0");
            listFiles(token, command);
        }
        else if (strcmp(token, "upload_file") == 0)
        {
            token = strtok(NULL, "\0");
            uploadFile(token, command);
        }
        else if (strcmp(token, "download_file") == 0)
        {
            token = strtok(NULL, "\0");
            downloadFile(token, command);
        }
        else if (strcmp(token, "show_downloads") == 0)
        {
            token = strtok(NULL, "\0");
            showDownloads();
        }
        else if (strcmp(token, "stop_share") == 0)
        {
            token = strtok(NULL, "\0");
            stopShare(token, command);
        }
        else if (command == "exit")
        {
            command += " " + curr_user.username;
            string response = sendRequest(tracker_socket, command);
            lock_guard<mutex> lock(client_mutex);
            isRunning = false;
            cout << "Exiting..." << endl;
            exit(1);
            break;
        }
        else
        {
            cout << "Invalid command." << endl;
        }
    }

    // recv_thread.join();

    close(tracker_socket);
    exit(0);
}