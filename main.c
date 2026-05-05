#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> //for close()

char *list_of_connections[100];
int loc = 0;

struct client {
    int sfd;
    char user_name[50];
};

//cleint finder
struct client* find_client(int fd, struct client clients[], int count) {
    for (int i = 0; i < count; i++) {
        if (clients[i].sfd == fd && clients[i].sfd != -1) //if that client is an fd and we didnt mark it as dead
            return &clients[i];
    }
    return NULL;
}

int main(int argc, char **argv) {

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    int sockfd;

    //for select()
    fd_set read_fds; //our master set
    fd_set working_set;
    int max_fd;

    //master_set  = all sockets I care about
    //working_set = sockets that are READY right now (after select)

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0; //i think?
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, "8080", &hints, &res);

    //now lets move through the linked list till one socket connects
    for (p = res; p != NULL; p = p->ai_next) {
        
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        int yes = 1; //this is just for like bypassing the OS time limit in between succsive like failed connections to localhost
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            return -1; 
        }

        if ((bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) { //here ai_addr is sockaddr struct struct
            close(sockfd); //close cuz bind failed 
            perror("server: bind");
            continue;
        }

        break; //found working struct -> bind succeeded.


    }

    if (p == NULL) {
        //nothign worked so we exit
        printf("Nothing in res worked, exiting...");
        return -1;
    }

    listen(sockfd, 10); //backlog queue len is 10
    //listen just turns the server on


    struct sockaddr_storage their_addr; //sockaddr_storage so we can fit ipv6 stuff too
    socklen_t their_socklen = sizeof(their_addr); //can be int also but this is better i think

    // char message[] = "hi from nirmal!\nWelcome to my chatroom... \n";

    //adding select logic
    //imp: select() modifies your fd_set!!

    //clear the set
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    max_fd = sockfd;
    

    // Only monitoring read_fds
    // this inside while loop
    
    
    char buffer[1024];

    struct client clients[100]; //for new 100 max clients
    int client_count = 0;


    //we gon multipelx on this shit
    while(1) {

        //clearing ze buffers
        memset(buffer, 0, sizeof(buffer));

        working_set = read_fds;
        int activity = select(max_fd + 1, &working_set, NULL, NULL, NULL);
        //select wakes up when a client connects, we set its FD as max_fd+1?

        if (activity > 0) {
            
            for (int i = 0; i <= max_fd; i++) {

                if (FD_ISSET(i, &working_set)) {

                    if (i == sockfd) {

                        //new connection has joined

                        int new_sock_fd = accept(sockfd, (struct sockaddr*)&their_addr, &their_socklen); //creates brand new FD
                        FD_SET(new_sock_fd, &read_fds);

                        if (new_sock_fd > max_fd)
                            max_fd = new_sock_fd;

                        //store the clients now
                        clients[client_count].sfd = new_sock_fd;
                        strcpy(clients[client_count].user_name, "test user");
                        client_count++;

                        send(new_sock_fd, "Enter username : ", strlen("Enter username : "), 0); 

                    }
                    
                    else {

                        struct client *c = find_client(i, clients, client_count);
                        //existing connection

                        int n = recv(i, buffer, sizeof(buffer), 0);

                        if (n <= 0) {
                            //error so remove that client
                            close(i);
                            FD_CLR(i, &read_fds);

                            // struct client *c = find_client(i, clients, client_count);
                            if (c) {
                                // Remove from list_of_connections
                                for (int m = 0; m < loc; m++) {
                                    if (list_of_connections[m] && strcmp(list_of_connections[m], c->user_name) == 0) {
                                        list_of_connections[m] = NULL;
                                        break;
                                    }
                                }
                                c->sfd = -1; //mark that client as dead
                            }

                            //broadcasting the online message again
                            char online_msg[512];
                            snprintf(online_msg, sizeof(online_msg), "ONLINE: ");
                            for (int k = 0; k < loc; k++) {
                                if (list_of_connections[k] != NULL) { //all the stuff that isnt marked as null we are printing
                                    strcat(online_msg, list_of_connections[k]);
                                    strcat(online_msg, " ");
                                }
                            }

                            strncat(online_msg, "\n", sizeof(online_msg) - strlen(online_msg) - 1);  //adding new line

                            //sending to every client
                            for (int j = 0; j <= max_fd; j++) {
                                if (FD_ISSET(j, &read_fds) && j != sockfd) {
                                    send(j, online_msg, strlen(online_msg), 0);
                                    // send(j, "\n", 1, 0);  this is buggy cuz send and recv are stream s
                                }
                            }

                            continue;

                        }

                        buffer[strcspn(buffer, "\n")] = 0; //removing new line if present

                        //if username not assigned
                        if (strcmp(c->user_name, "test user") == 0) {
                            
                            strcpy(c->user_name, buffer);

                            list_of_connections[loc++] = c->user_name;

                            char online_msg[512];
                            snprintf(online_msg, sizeof(online_msg), "ONLINE: ");
                            for (int k = 0; k < loc; k++) { 
                                if (list_of_connections[k] != NULL) { //same as before, whatver we didnt mark as null we print
                                    strcat(online_msg, list_of_connections[k]);
                                    strcat(online_msg, " ");
                                }
                            }
                            
                            strncat(online_msg, "\n", sizeof(online_msg) - strlen(online_msg) - 1);  //adding new line

                            //sending to every client
                            for (int j = 0; j <= max_fd; j++) {
                                if (FD_ISSET(j, &read_fds) && j != sockfd) {
                                    send(j, online_msg, strlen(online_msg), 0);
                                    // send(j, "\n", 1, 0);  this is buggy cuz send and recv are stream s
                                }
                            }
                        }
                        
                        else {

                            //i is sender, rest all -> j loop is recivier

                            for (int j = 0; j <= max_fd; j++) {
                                if (FD_ISSET(j, &read_fds)) {
                                    if (j != sockfd && j != i) {
                                        char reply[1024];
                                        snprintf(reply, sizeof(reply), "%s : %s\n", c->user_name, buffer);
                                        send(j, reply, strlen(reply), 0);
                                    }

                                }
            
                            }

                        }

                    }

                }

            }

        }

    }
    
    close(sockfd);
    freeaddrinfo(res); //all done with addr cuz we got the socket? 


    return 0;
}
