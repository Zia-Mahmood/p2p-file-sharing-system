#include "client.h"

// Handling create_user input
void createUser(char *data, string command)
{
    // Create user
    string username, password;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for creating user. username and password are required\n";
        return;
    }
    username = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for creating user. password is required\n";
        return;
    }
    password = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for creating user. correct command is: create_user <user_id> <passwd>\n";
    }
    else
    {
        string response = sendRequest(tracker_socket, command + " " + string(client_ip) + " " + to_string(client_port));
        if (strcmp(response.c_str(), "user_created_successfully") == 0)
        {
            cout << "User " << username << " created successfully." << endl;
        }
        else if (strcmp(response.c_str(), "user_already_exists") == 0)
        {
            cout << "User " << username << " already exists!" << endl;
        }
    }
}

// Handling login input
void login(char *data, string command)
{
    string username, password;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for user login. username and password are required\n";
        return;
    }
    username = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for user login. password is required\n";
        return;
    }
    password = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for user login. correct command is: login <user_id> <passwd>\n";
    }
    else if (!isLoggedIn())
    {
        string response = sendRequest(tracker_socket, command + " " + string(client_ip) + " " + to_string(client_port));

        if (strcmp(response.c_str(), "user_is_already_logged_in") == 0)
        {
            cout << "User " << username << " is already logged in." << endl;
        }
        else if (strcmp(response.c_str(), "invalid_username_password") == 0)
        {
            cout << "Invalid username or password." << endl;
        }
        else
        {
            // cout << response << endl;
            if (response.size() > 0 && response != " ")
            {
                // cout << "response on relogin: " << response << endl;
                curr_user.deserialize(response);
            }
            curr_user.username = username;
            curr_user.password = password;
            curr_user.is_logged_in = true;
            cout << "User " << username << " logged in successfully." << endl;
        }
    }
    else
    {
        cout << "A User " << curr_user.username << " is already logged in. Can't login two users at a time." << endl;
    }
}

// Handling logout input
void logout(char *data, string command)
{
    char *token = strtok(data, " ");
    if (token != NULL)
    {
        cout << "Invalid command for user logout. correct command is: logout\n";
    }

    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        // cout << "logout response: " << response << endl;
        if (strcmp(response.c_str(), "user_logged_out") == 0)
        {
            curr_user.is_logged_in = false;
            cout << "User " << curr_user.username << " logged out successfully." << endl;
        }
    }
}

// Handling create_group input
void createGroup(char *data, string command)
{
    string group_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for creating group. <group_id> is required\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for creating group. correct command is: create_group <group_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (strcmp(response.c_str(), "group_created_successfully") == 0)
        {
            cout << "Group " << group_id << " created successfully." << endl;
        }
        else if (strcmp(response.c_str(), "group_already_exists") == 0)
        {
            cout << "Group " << group_id << " already exists!" << endl;
        }
    }
}

// Handling join_group input
void joinGroup(char *data, string command)
{
    string group_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for joining group request. <group_id> is required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for joining group request. correct command is: join_group <group_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (strcmp(response.c_str(), "user_requested_to_join_group") == 0)
        {
            cout << "User " << curr_user.username << " requested to join group " << group_id << " successfully." << endl;
        }
        else if (strcmp(response.c_str(), "join_request_already_exist") == 0)
        {
            cout << "User " << curr_user.username << " has already requested for joining group " << group_id << ".\n";
        }
        else if (strcmp(response.c_str(), "user_already_member_of_group") == 0)
        {
            cout << "User " << curr_user.username << " is already a member of group " << group_id << "." << endl;
        }
        else if (strcmp(response.c_str(), "group_not_exist") == 0)
        {
            cout << "Group " << group_id << " does not exist." << endl;
        }
    }
}

// Handling leave_group input
void leaveGroup(char *data, string command)
{
    string group_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for leaving a group. <group_id> is required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for leaving a group. correct command is: leave_group <group_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (strcmp(response.c_str(), "user_left_group_successfully") == 0)
        {
            cout << "User " << curr_user.username << " left group " << group_id << " successfully." << endl;
        }
        else if (strcmp(response.c_str(), "user_not_member_of_group") == 0)
        {
            cout << "User " << curr_user.username << " is not a member of group " << group_id << "." << endl;
        }
        else if (strcmp(response.c_str(), "group_not_exist") == 0)
        {
            cout << "Group " << group_id << " does not exist." << endl;
        }
    }
}

// Handling list_requests input
void listRequests(char *data, string command)
{
    string group_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for listing requests for a joining group. <group_id> is required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for listing requests for a joining group. correct command is: list_requests <group_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (strcmp(response.c_str(), "user_not_authorized_to_see_pending_request_for_this_group") == 0)
        {
            cout << "User " << curr_user.username << " is not authorized to see pending requests of group " << group_id << " because he is not the owner of this group." << endl;
        }
        else if (strcmp(response.c_str(), "group_not_exist") == 0)
        {
            cout << "Group " << group_id << " does not exist." << endl;
        }
        else if (response.length() > 0 && response != " ")
        {
            char *ptr = strtok((char *)response.c_str(), " ");
            int i = 1;
            cout << "List of request for joining group " << group_id << " :" << endl;
            while (ptr != NULL)
            {
                cout << i << ") Username (user_id): " << ptr << endl;
                ptr = strtok(NULL, " ");
                i++;
            }
            cout << endl;
        }
        else
        {
            cout << "No Pending request for joining group " << group_id << "." << endl;
        }
    }
}

// Handling accept_request input
void acceptRequest(char *data, string command)
{
    string group_id, user_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for accepting a user in a group. <group_id> and <user_id> are required\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for accepting a user in a group. <user_id> is required\n";
        return;
    }
    user_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for accepting a user in a group. correct command is: accept_request <group_id> <user_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (strcmp(response.c_str(), "user_added_successfully") == 0)
        {
            cout << "User " << user_id << " has been added to group " << group_id << "." << endl;
        }
        else if (strcmp(response.c_str(), "user_already_member_of_group") == 0)
        {
            cout << "User " << user_id << " is already member of group " << group_id << ".\n";
        }
        else if (strcmp(response.c_str(), "user_not_requested_to_join_group") == 0)
        {
            cout << "User " << user_id << " did not requested to join group" << group_id << ".\n";
        }
        else if (strcmp(response.c_str(), "user_not_authorized_to_accept_members_for_this_group") == 0)
        {
            cout << "User " << curr_user.username << " is not authorized to accept members of group " << group_id << " because he is not the owner of this group." << endl;
        }
        else if (strcmp(response.c_str(), "group_not_exist") == 0)
        {
            cout << "Group " << group_id << " does not exist." << endl;
        }
    }
}

// Handling reject_request input
void rejectRequest(char *data, string command)
{
    string group_id, user_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for rejecting user request for joining a group. <group_id> and <user_id> are required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        cout << "Invalid command for rejecting user request for joining a group. <user_id> is required.\n";
        return;
    }
    user_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for rejecting user request for joining a group. correct command is: reject_request <group_id> <user_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (strcmp(response.c_str(), "user_rejected_successfully") == 0)
        {
            cout << "User " << user_id << " request to join has been rejected by the owner of group " << group_id << "." << endl;
        }
        else if (strcmp(response.c_str(), "user_already_member_of_group") == 0)
        {
            cout << "User " << user_id << " is already member of group " << group_id << ".\n";
        }
        else if (strcmp(response.c_str(), "user_not_requested_to_join_group") == 0)
        {
            cout << "User " << user_id << " did not requested to join group" << group_id << ".\n";
        }
        else if (strcmp(response.c_str(), "user_not_authorized_to_accept_members_for_this_group") == 0)
        {
            cout << "User " << curr_user.username << " is not authorized to accept members of group " << group_id << " because he is not the owner of this group." << endl;
        }
        else if (strcmp(response.c_str(), "group_not_exist") == 0)
        {
            cout << "Group " << group_id << " does not exist." << endl;
        }
    }
}

// Handling list_groups input
void listGroups(char *data, string command)
{
    char *token = strtok(data, " ");
    if (token != NULL)
    {
        cout << "Invalid command for listing all groups. correct command is: list_groups\n";
    }
    else if (isLoggedIn())
    {
        string response = sendRequest(tracker_socket, command);
        if (response.length() > 0 && response != " ")
        {
            char *ptr = strtok((char *)response.c_str(), " ");
            int i = 1;
            cout << "List of all groups in the network:" << endl;
            while (ptr != NULL)
            {
                cout << i << ") Group ID: " << ptr << endl;
                ptr = strtok(NULL, " ");
                i++;
            }
            cout << endl;
        }
        else
        {
            cout << "No groups present in the network." << endl;
        }
    }
}

// Handling list_my_groups input
void listMyGroups(char *data, string command)
{
    char *token = strtok(data, " ");
    if (token != NULL)
    {
        cout << "Invalid command for listing all groups where current user is added. correct command is: list_my_groups\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (response.length() > 0 && response != " ")
        {
            char *ptr = strtok((char *)response.c_str(), " ");
            int i = 1;
            cout << "List of all groups where user " << curr_user.username << " is part of, in the network : " << endl;
            while (ptr != NULL)
            {
                cout << i << ") Group ID: " << ptr << endl;
                ptr = strtok(NULL, " ");
                i++;
            }
            cout << endl;
        }
        else
        {
            cout << "User " << curr_user.username << " is not part of any group present in the network." << endl;
        }
    }
}

// Function to handle listing of files
void listFiles(char *data, string command)
{
    string group_id;
    char *token = strtok(data, " ");
    if (token == NULL)
    {
        cout << "Invalid command for listing shared files of a group. <group_id> is required.\n";
        return;
    }
    group_id = token;
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        cout << "Invalid command for listing shared files of a group. correct command is: list_files <group_id>\n";
    }
    else if (isLoggedIn())
    {
        command += " " + curr_user.username;
        string response = sendRequest(tracker_socket, command);
        if (strcmp(response.c_str(), "user_not_member_of_group") == 0)
        {
            cout << "User " << curr_user.username << " is not member of the group " << group_id << endl;
        }
        else if (strcmp(response.c_str(), "group_not_exist") == 0)
        {
            cout << "Group " << group_id << " does not exist." << endl;
        }
        else if (response.length() > 0 && response != " ")
        {
            char *ptr = strtok((char *)response.c_str(), " ");
            int i = 1;
            cout << "List of shared files of group " << group_id << " :" << endl;
            while (ptr != NULL)
            {
                cout << i << ") File name : " << ptr << endl;
                ptr = strtok(NULL, " ");
                i++;
            }
            cout << endl;
        }
        else
        {
            cout << "No File shared in this group " << group_id << "." << endl;
        }
    }
}
