#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<ncurses.h>
#include<unistd.h> //for close()

int main(int argc, char **argv) {

    initscr();
    noecho();
    cbreak();

    int ymax, xmax;

    getmaxyx(stdscr, ymax, xmax);

    WINDOW *main_win = newwin(ymax, xmax, 0, 0);
    WINDOW *chat_win = newwin(ymax - 3, xmax, 0, 0);
    WINDOW *message_win = newwin(3, xmax, ymax - 3, 0);
    WINDOW *members_win = newwin(ymax - 3, 20, 0, xmax - 20);

    keypad(message_win, TRUE); //allows arrow keys

    refresh();

    WINDOW *scroll_win = newwin(ymax-5, xmax-23, 1, 1);
    scrollok(scroll_win, TRUE);

    box(main_win, 0, 0);
    box(chat_win, 0, 0);
    box(message_win, 0, 0);

    struct sockaddr_in server_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhost, hardcoding ipv4

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect error");
        exit(1);
    }

    char previous_line[1024] = " ";

    fd_set read_fds;
    int max_fd = (sockfd > 0) ? sockfd : 0;

    char buffer[1024];

    while(1) {

        memset(buffer, 0, sizeof(buffer));

        //dont need working set and all cuz we know we only have two fixed items in the set
        //no dynamic number of FDs stuff in here.

        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds); //0 is STDIN
        FD_SET(sockfd, &read_fds);

        wrefresh(chat_win);
        wrefresh(message_win);
        wrefresh(members_win);

        if (select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(0, &read_fds)) {
            //STDIN has activity
            // fgets(buffer, sizeof(buffer), stdin);

            //clearing the window again 

            int ch = wgetch(message_win);

            if (ch == KEY_UP) {
                // show previous message
                wmove(message_win, 1, 1);
                wclrtoeol(message_win);
                box(message_win, 0, 0);
                wrefresh(message_win);

                int len = strlen(previous_line);
                for (int i = len - 1; i >= 0; i--) { //putting the previous line buffer backinto the input queue 
                    ungetch(previous_line[i]);
                }

                //cleanly sending the prev stuff now
                echo();
                mvwgetnstr(message_win, 1, 1, buffer, 1000); //enter key blocking and for somereason it reads fresh from user
                noecho();

                send(sockfd, buffer, strlen(buffer), 0);
                strncpy(previous_line, buffer, sizeof(previous_line));  //to keep the current line, history of 1 message

                wprintw(scroll_win, "you : %s\n", buffer);
                wrefresh(scroll_win);

                wmove(message_win, 1, 1);
                wclrtoeol(message_win);
                box(message_win, 0, 0); 

            }

            else {

                ungetch(ch); //if not arrow key then we put it back for mvwgetnstr
                    
                wmove(message_win, 1, 1);
                wclrtoeol(message_win);
                box(message_win, 0, 0);

                echo();
                mvwgetnstr(message_win, 1, 1, buffer, 1000);
                noecho();
                send(sockfd, buffer, strlen(buffer), 0);

                strncpy(previous_line, buffer, sizeof(previous_line));

                wprintw(scroll_win, "you : %s\n", buffer);
                wrefresh(scroll_win);

                //cleaning up after eating like the good manners my mom taught me
                wmove(message_win, 1, 1);
                wclrtoeol(message_win);
                box(message_win, 0, 0);
            }
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            //SOCKFD has activity
            int n = recv(sockfd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                //connection closed;
                break;
            }

            buffer[n] = '\0'; 

            //splitng on \n and getting each message
            char *line = strtok(buffer, "\n");
            while (line != NULL) {
                    
                if (strncmp(line, "ONLINE:", 7) == 0) {

                    wclear(members_win);
                    box(members_win, 0, 0);
                    mvwprintw(members_win, 1, 1, "%s\n", "Online : ");
                    int row = 2;
                    char copy[512];
                    strncpy(copy, buffer+7, sizeof(copy)); //skipping the word online:
                    copy[strcspn(copy, "\n")] = 0;
                    char *token = strtok(copy, " ");
                    while (token) {
                        mvwprintw(members_win, row++, 1, "%s", token);
                        token = strtok(NULL, " "); //null means continue where we left off
                    }
                    wrefresh(members_win);
                }
                else {
                    wprintw(scroll_win, "%s\n", line);
                    wrefresh(scroll_win);
                }
                line = strtok(NULL, "\n");
            }
        }

    }

    close(sockfd);

    getch();
    endwin();


    return 0;
}