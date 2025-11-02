#include "client.h"

// Handling upload_file input
void uploadFile(char *data, string command)
{
    string file_full_path, group_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for uploading file. <file_path> is required.\n";
        return;
    }
    file_full_path = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for uploading file. <group_id> is required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for uploading file to a group. correct command is: upload_file <file_path> <group_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        vector<string> hash;
        string hashes = initializeHashes(file_full_path, hash);

        if (hashes.size() > 0)
        {
            string file_name, file_path;
            separateFilePath(file_full_path, file_path, file_name);
            if (curr_user.shared_files.find(file_name) == curr_user.shared_files.end())
            {
                command += " " + hashes;
                // cout << "Final command: " << command << endl;
                string response = sendRequest(tracker_socket, command);
                // cout << "upload response: " << response << endl;
                if (strcmp(response.c_str(), "file_uploaded_successfully") == 0)
                {
                    curr_user.shared_files[file_name] = file_path;
                    Fileinfo new_file = {"", group_id, true, false, true, "", {}, {}, {}};
                    new_file.file_path = file_path;
                    new_file.master_hash = hash[0];
                    vector<string> ofc(hash.begin() + 1, hash.end());
                    new_file.order_of_chunks = ofc;
                    for (string sha : new_file.order_of_chunks)
                    {
                        new_file.chunk_map[sha] = {};
                        new_file.hashed_file[sha] = "";
                    }
                    files[file_name] = new_file;
                    cout << "File: " << file_name << " is now being shared by " << curr_user.username << " in group " << group_id << endl;
                }
                else if (strcmp(response.c_str(), "user_not_member_of_group") == 0)
                {
                    cout << "User " << curr_user.username << "is not member of the group " << group_id << endl;
                }
                else if (strcmp(response.c_str(), "group_not_exist") == 0)
                {
                    cout << "Group " << group_id << " does not exist." << endl;
                }
                else if (strcmp(response.c_str(), "incorrect_file_uploaded") == 0)
                {
                    cout << "Hashes of file already, present and current file doesn't match" << endl;
                }
                else if (strcmp(response.c_str(), "user_already_sharing_this_file") == 0)
                {
                    cout << "User " << curr_user.username << " is already sharing file " << file_name << " in the group " << group_id << endl;
                }
            }
            else if (!files[file_name].is_sharing)
            {
                cout << "file " << file_name << " was not been shared temporarily. Will restart sharing now." << endl;
                files[file_name].is_sharing = true;
            }
            else
            {
                cout << "File is already shared by the user" << endl;
            }
        }
    }
}

// Function to get available chunks info from all sharing clients
void getChunksAvailable(string peer, const string &master_hash, const string &file_name)
{
    lock_guard<mutex> lock(client_mutex);
    char ip[16];
    string new_peer = peer;
    char *token = strtok((char *)new_peer.c_str(), ":");
    strcpy(ip, token);
    token = strtok(NULL, "\0");
    int port = stoi(string(token));
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        cout << "Failed to connect to client with ip : " << ip << " port " << port << endl;
        return;
    }

    struct sockaddr_in tracker_addr;
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &tracker_addr.sin_addr) <= 0)
    {
        cout << "Failed to connect to client with ip : " << ip << " port " << port << endl;
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0)
    {
        cout << "Failed to connect to client with ip : " << ip << " port " << port << endl;
        close(sock);
        return;
    }
    string response = sendRequest(sock, "have_file " + master_hash);
    // cout << "response from client with ip : " << ip << " port " << port << " : " << response << endl;
    if (response == "yes_complete_file_available")
    {
        for (auto &[hash, peerVector] : files[file_name].chunk_map)
        {
            peerVector.push_back(peer);
        }
    }
    else if (response == "user_is_not_logged_in")
    {
        cout << "user is not available currently ip : " << ip << " port " << port << endl;
    }
    else if (response == "file_not_available")
    {
        cout << "users doesn't have any chunks with him ip : " << ip << " port " << port << endl;
    }
    else
    {
        char *token = strtok((char *)response.c_str(), " ");
        while (token != NULL)
        {
            int i = stoi(token);
            files[file_name].chunk_map[files[file_name].order_of_chunks[i]].push_back(peer);
            token = strtok(NULL, " ");
        }
    }
    close(sock);
}

// Function to download chunks from peers using piece wise algorithm
void getChunks(vector<string> peers, string master_hash, string chunk_hash, string file_name, string group_id)
{
    lock_guard<mutex> lock(client_mutex);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, peers.size() - 1);

    while (true && peers.size() > 0)
    {
        int random_peer_index = dist(gen);
        string peer = peers[random_peer_index];
        char ip[16];
        string new_peer = peer;
        char *token = strtok((char *)new_peer.c_str(), ":");
        strcpy(ip, token);
        token = strtok(NULL, "\0");
        int port = stoi(string(token));
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        int buffer_size = 524300; // 512 KB
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
        if (sock < 0)
        {
            cout << "Failed to connect to client with ip : " << ip << " port " << port << endl;
            continue;
        }

        struct sockaddr_in tracker_addr;
        tracker_addr.sin_family = AF_INET;
        tracker_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip, &tracker_addr.sin_addr) <= 0)
        {
            cout << "Failed to connect to client with ip : " << ip << " port " << port << endl;
            close(sock);
            continue;
        }

        if (connect(sock, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0)
        {
            cout << "Failed to connect to client with ip : " << ip << " port " << port << endl;
            close(sock);
            continue;
        }
        string response = sendRequest(sock, "send_chunk " + master_hash + " " + chunk_hash + " " + file_name);
        if (response == "user_is_not_logged_in")
        {
            cout << "user is not available currently ip : " << ip << " port " << port << endl;
        }
        else if (response == "file_not_available")
        {
            cout << "users doesn't have the file with him ip : " << ip << " port " << port << endl;
        }
        else if (response == "user_not_sharing_currently")
        {
            cout << "users doesn't want to share file with us, he choked us!! ip : " << ip << " port " << port << endl;
        }
        else if (response == "incorrect_hash")
        {
            cout << "incorrect hash received ip : " << ip << " port " << port << endl;
        }
        else if (response == "currently_this_chunk_is_not_available")
        {
            cout << "users doesn't have the this chunk with him at present ip : " << ip << " port " << port << endl;
        }
        else
        {
            string new_hash = computeSHA1(response);
            // cout << new_hash << endl;
            if (new_hash == chunk_hash)
            {
                cout << "received chunk from " << peer << " chunk hash " << chunk_hash << endl;
                if (!files[file_name].is_sharing)
                {
                    string response = sendRequest(tracker_socket, "upload_file " + files[file_name].file_path + "/" + file_name + " " + group_id + " " + curr_user.username + " " + master_hash);
                    if (response == "user_already_sharing_this_file")
                    {
                        // cout << "User " << curr_user.username << " is already sharing file " << file_name << " in the group " << group_id << endl;
                    }
                    else if (response == "file_uploaded_successfully")
                    {
                        curr_user.shared_files[file_name] = files[file_name].file_path;
                        files[file_name].is_sharing = true;
                        cout << "Notified the tracker that I am also sharing file: " << file_name << " " << files[file_name].is_sharing << endl;
                    }
                }
                files[file_name].hashed_file[chunk_hash] = response;
                close(sock);
                break;
            }
        }
        this_thread::sleep_for(chrono::seconds(5));
        close(sock);
    }
}

// Function to handle download input
void downloadFile(char *data, string command)
{
    string group_id, file_name, file_path;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for downloading a file. <group_id> is required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for downloading a file. <file_name> is required.\n";
        return;
    }
    file_name = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for downloading a file. <destination_path> is required.\n";
        return;
    }
    file_path = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for downloading a file from a group. correct command is: <group_id> <file_name> <destination_path>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, "download_file " + group_id + " " + file_name + " " + curr_user.username);
        if (strcmp(response.c_str(), "user_not_member_of_group") == 0)
        {
            cout << "User " << curr_user.username << "is not member of the group " << group_id << endl;
        }
        else if (strcmp(response.c_str(), "group_not_exist") == 0)
        {
            cout << "Group " << group_id << " does not exist." << endl;
        }
        else if (strcmp(response.c_str(), "file_not_present_for_sharing") == 0)
        {
            cout << "File " << file_name << " is not present in the group " << group_id << endl;
        }
        else if (files.find(file_name) != files.end() && files[file_name].is_complete && files[file_name].file_path == file_path)
        {
            cout << "Already complete file is present at the destination location: " << file_path << "/" << file_name << endl;
        }
        else
        {
            vector<string> hashes;
            vector<string> peers;
            deserialize(response, hashes, peers);
            Fileinfo new_file = {file_path, group_id, false, true, false, hashes[0], {}, {}, {}};
            vector<string> ofc(hashes.begin() + 1, hashes.end());
            new_file.order_of_chunks = ofc;
            for (string sha : new_file.order_of_chunks)
            {
                new_file.chunk_map[sha] = {};
                new_file.hashed_file[sha] = "";
            }
            files[file_name] = new_file;
            int i = 0;
            cout << "Hashes of the file received from tracker:" << endl;
            for (string hash : hashes)
            {
                cout << "Hash " << i << " : " << hash << endl;
                i++;
            }
            i = 0;
            vector<thread> threads;
            for (string peer : peers)
            {
                cout << "Sending request to get all available chunks with Peer " << i << " : " << peer << endl;
                threads.push_back(thread(getChunksAvailable, peer, hashes[0], file_name));
                i++;
            }
            for (auto &t : threads)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }

            vector<pair<string, vector<string>>> chunk_vector(files[file_name].chunk_map.begin(), files[file_name].chunk_map.end());

            sort(chunk_vector.begin(), chunk_vector.end(),
                 [](const pair<string, vector<string>> &a, const pair<string, vector<string>> &b)
                 {
                     return a.second.size() < b.second.size();
                 });

            cout << "Chunks to peer mapping: (info about which chunk is with which peer.)" << endl;
            for (size_t j = 0; j < files[file_name].order_of_chunks.size(); j++)
            {
                cout << "hash " << j << " : " << files[file_name].order_of_chunks[j] << " : ";
                for (string s : files[file_name].chunk_map[files[file_name].order_of_chunks[j]])
                {
                    cout << s << " ";
                }
                cout << endl;
            }

            i = 0;
            for (auto &[chunk_hash, peers] : chunk_vector)
            {
                cout << "Sending request to download chunk with hash " << i << " : " << chunk_hash << endl;

                thread getChunksthread(getChunks, peers, files[file_name].master_hash, chunk_hash, file_name, group_id);
                getChunksthread.join();
                i++;
            }
            cout << "All chunks accumulated." << endl;
            string completedata;
            for (size_t i = 0; i < files[file_name].order_of_chunks.size(); ++i)
            {
                const string &hash = files[file_name].order_of_chunks[i];

                // Find the chunk data in the hashed_file map
                if (files[file_name].hashed_file.find(hash) == files[file_name].hashed_file.end())
                {
                    cout << "\n!!ERROR!!\nChunk not found for hash: " << hash << endl;
                    return;
                }

                completedata += files[file_name].hashed_file[hash];
            }
            if (computeSHA1(completedata) == files[file_name].master_hash)
            {
                cout << "SHA1 for complete file is also verified." << endl;
            }

            if (!constructFile(file_name))
            {
                cout << "Failed to download the file. Try Again after sometime \n";
                return;
            }
            files[file_name].is_complete = true;
            for (string hash : files[file_name].order_of_chunks)
            {
                files[file_name].chunk_map[hash] = {};
                files[file_name].hashed_file[hash] = "";
            }
            // cout << response << endl;
        }
    }
}

// Function to construct the downloaded file.
bool constructFile(string file_name)
{
    // Ensure directory exists
    if (!directoryExists(files[file_name].file_path))
    {
        cout << "\n!!ERROR!!\nDirectory does not exist: " << files[file_name].file_path << endl;
        return false;
    }

    string outputFilePath = files[file_name].file_path + "/" + file_name;
    struct stat buff;
    if (stat(outputFilePath.c_str(), &buff) == 0)
    {
        cout << "\n!!ERROR!!\nFile already exists: " << outputFilePath << endl;
        return false;
    }

    int outputFile = open(outputFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (outputFile == -1)
    {
        cout << "\n!!ERROR!!\nFailed to open output file: " << outputFilePath << endl;
        return false;
    }

    const size_t bufferSize = 524288;
    char *buffer = new char[bufferSize];

    // Iterate through the chunks in order of creation
    size_t total_bytes_written = 0;
    for (size_t i = 0; i < files[file_name].order_of_chunks.size(); ++i)
    {
        const string &hash = files[file_name].order_of_chunks[i];

        // Find the chunk data in the hashed_file map
        if (files[file_name].hashed_file.find(hash) == files[file_name].hashed_file.end())
        {
            cout << "\n!!ERROR!!\nChunk not found for hash: " << hash << endl;
            return false;
        }

        const string &chunkData = files[file_name].hashed_file[hash];

        // Write the chunk data to the output file
        size_t chunkSize = chunkData.size();
        if (write(outputFile, chunkData.c_str(), chunkSize) != static_cast<ssize_t>(chunkSize))
        {
            cout << "\n!!ERROR!!\nFailed to write chunk to output file\n";
            return false;
        }

        total_bytes_written += chunkSize;

        // Display progress
        double progress = static_cast<double>(i + 1) / files[file_name].order_of_chunks.size();
        printProgressBar(progress);
    }

    cout << "\nFile successfully Downloaded. Total bytes written: " << total_bytes_written << endl;

    // Clean up resources
    close(outputFile);
    delete[] buffer;

    return true;
}

// Helper function to check if a directory exists
bool directoryExists(const string &dirPath)
{
    struct stat info;
    if (stat(dirPath.c_str(), &info) != 0)
        return false;                     // Cannot access path
    return (info.st_mode & S_IFDIR) != 0; // Check if it's a directory
}

// printing progress bar of percentage file written on shell
void printProgressBar(double progress)
{
    size_t width = 100;
    size_t filledWidth = static_cast<size_t>(progress * width);

    string progressBar = "\rWriting: [";

    progressBar.append(filledWidth, '#');
    progressBar.append(width - filledWidth, '.');

    progressBar.append("] ");

    progressBar.append(to_string(static_cast<int>(progress * 100)));
    progressBar.append("% done");

    string coloredBar = "\033[32m" + progressBar + "\033[0m"; // adding green color to the final output

    write(STDERR_FILENO, coloredBar.c_str(), coloredBar.length());
    fflush(stderr); // flushing the previous line
}

// Function to deserialize the response for download input
void deserialize(const string &response, vector<string> &hashes, vector<string> &peers)
{
    stringstream ss(response);
    string token;
    getline(ss, token, '$');
    stringstream groupsStream(token);
    while (getline(groupsStream, token, ','))
    {
        if (!token.empty())
        {
            hashes.push_back(token);
        }
    }

    while (getline(ss, token, ','))
    {
        if (!token.empty())
        {
            peers.push_back(token);
        }
    }
}

// Function to show Downloads
void showDownloads()
{
    lock_guard<mutex> lock(client_mutex);
    int i = 1;
    cout << "Showing downloads: " << endl;
    for (const auto &[id, file] : files)
    {
        if (file.is_download_file && file.is_complete)
        {
            cout << i << ") [C] " << file.group_id << " " << id << endl;
            i++;
        }
        else if (file.is_download_file && !file.is_complete)
        {
            cout << i << ") [D] " << file.group_id << " " << id << endl;
            i++;
        }
    }
    if (i == 1)
    {
        cout << "No files are downloading or has completed download" << endl;
    }
    else
    {
        cout << "D (Downloading), C (Complete)" << endl;
    }
}

// Function to stop sharing file in group
void stopShare(char *data, string command)
{
    string file_name, group_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for stop sharing file. <group_id> is required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for stop sharing file. <file_name> is required.\n";
        return;
    }
    file_name = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for stop sharing a file in a group. correct command is: stop_share <group_id> <file_name>\n";
    }
    else if (isLoggedIn())
    {
        if (files.find(file_name) != files.end())
        {
            files[file_name].is_sharing = false;
            cout << "Stopped sharing file " << file_name << " will restarting sharing on next request from peers." << endl;
        }
        else
        {
            cout << "User is has not uploaded or downloaded this file" << endl;
        }
    }
}