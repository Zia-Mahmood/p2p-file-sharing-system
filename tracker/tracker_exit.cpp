#include "tracker.h"

// Signal handler to catch the termination signal and perform clean-up
void handleSignal()
{
    cout << "Tracker is going offline. Notifying other trackers...\n";
    // Notify other trackers before going offline
    for (const auto &[id, tracker] : trackers)
    {
        int count = 1;
        while (id != curTrackerId && tracker.is_online && count <= 3)
        {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                // cout << "Error creating socket for sending OFFLINE.\n";
                // cout << "Failed to tell tracker: " << id << " Attempt " << count << " failed." << endl;
                count++;
                continue;
            }

            struct sockaddr_in tracker_addr;
            tracker_addr.sin_family = AF_INET;
            tracker_addr.sin_port = htons(tracker.port);

            if (inet_pton(AF_INET, tracker.ip, &tracker_addr.sin_addr) <= 0)
            {
                cout << "Invalid address for tracker: " << tracker.ip << endl;
                close(sock);
                break;
            }

            if (connect(sock, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) >= 0)
            {
                string message = "OFFLINE " + to_string(curTrackerId);
                send(sock, message.c_str(), message.size(), 0);
                cout << "Informed tracker: " << id << endl;
                close(sock);
                break;
            }
            // cout << "Failed to tell tracker: " << id << " Attempt " << count << " failed." << endl;
            count++;
            close(sock);
        }
    }
    string shutdown_message = "shutting_down " + to_string(curTrackerId);

    for (const auto &[id, user] : users)
    {
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
                send(sock, shutdown_message.c_str(), shutdown_message.size(), 0);
                cout << "Shutdown notification sent to user at " << user.ip << ":" << user.port << endl;
                close(sock);
                break;
            }
            // cout << "Failed to connect to user " << user.ip << ":" << user.port << " Attempt " << count << " failed." << endl;
            count++;
            close(sock);
        }
    }
    isRunning = false; // Set flag to terminate the server loop
}

// Function to monitor for 'exit' command
void monitorExitCommand()
{
    string command;
    while (isRunning)
    {
        getline(cin, command);
        if (command == "exit")
        {
            handleSignal();
            cout << "Tracker has shut down." << endl;
            exit(0);
        }
    }
}

// Function to handle when other trackers go offline
void trackerIsOffline(int id)
{
    lock_guard<mutex> lock(user_group_mutex);
    trackers[id].is_online = false;
}