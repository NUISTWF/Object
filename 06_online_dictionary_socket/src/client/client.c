#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <proto.h>

/**
 * @brief Registering functions
 * 
 * @param sd Socket descriptor
 * @return None
 * @note None
 */
static void do_register(int sd)
{
    MSG cur;

    printf("register ...\n");
    printf("Please enter your username and password.\n");

    cur.type = htonl(R);    // 4 bytes need to be converted to network byte order
    scanf("%s", cur.name);
    scanf("%s", cur.data);
    printf("Information entered successfully and you usr:%s, password:%s.\n", cur.name, cur.data);

    // send() menu1 information
    if (send(sd, &cur, sizeof(cur), 0) < 0) {
        perror("send() menu1");
        exit(1);
    }

    // recv() waiting for the server's response
    if (recv(sd, &cur, sizeof(cur), 0) < 0) {
        perror("recv() menu1");
        exit(1);
    }

    printf("register (%s).\n", cur.data);

    return;
}

/**
 * @brief Login Function
 * 
 * @param msg Backfill message
 * @param sd Socket descriptor
 * @return If successful returns 0; fails returns 1
 * @note Login requires sending personal information to the server for verification
 */
static int do_login(MSG* msg, int sd)
{
    MSG cur;

    printf("login ...\n");
    printf("Please enter your username and password.\n");

    cur.type = htonl(L);
    scanf("%s", cur.name);
    scanf("%s", cur.data);
    // printf("Information entered successfully and you usr:%s, password:%s.\n", cur.name, cur.data);
    *msg = cur;

    // send() person information
    if (send(sd, &cur, sizeof(cur), 0) < 0) {
        perror("client send() person information");
        exit(1);
    }

    // recv() waiting for the server's response
    if (recv(sd, &cur, sizeof(cur), 0) < 0) {
        perror("client recv() person information response");
        exit(1);
    }
    // printf("information type:%d, name:%s, data:%s.\n", ntohl(cur.type), cur.name, cur.data);

    if (!strcmp(cur.data, "ok")) {
        return 0;
    } else {
        printf("Username or password is incorrect!\n");
        return 1;
    }
}

/**
 * @brief Query word and description
 * 
 * @param msg Message
 * @param sd Socket descriptor
 * @return None
 * @note Get the backfilled user information and send it to the server to record the query word record
 */
static void do_query(MSG* msg, int sd)
{
    MSG cur, rcvMsg;

    printf("query ...\n");
    cur.type = htonl(Q);
    strcpy(cur.name, msg->name);

    // Keep searching for words in a loop, # means stop searching
    while (1) {
        memset(&rcvMsg, 0, sizeof(rcvMsg));
        printf("Please enter your query word('#' quit):\n");
        scanf("%s", cur.data);  // At this time, name saves the user name
        if (!strcmp(cur.data, "#"))
            break;
        
        if (send(sd, &cur, sizeof(cur), 0) < 0) {
            perror("client send() query word!");
            exit(1);
        }

        if (recv(sd, &rcvMsg, sizeof(rcvMsg), 0) < 0) {
            perror("client recv() person information response");
            exit(1);
        }

        if (!strcmp(rcvMsg.name, "ok")) {
            printf("%s:%s.\n", cur.data, rcvMsg.data);
        } else if (!strcmp(rcvMsg.name, "no")) {
            printf("The word was not found!\n");
        }
    }
    
    return;
}

/**
 * @brief View History
 * @param sd Socket descriptor
 * @param msg Message
 * @return None
 * @note Query history records based on personal information
 */
static void do_history(MSG* msg, int sd)
{
    MSG cur, rcvMsg;

    cur.type = htonl(H);
    strcpy(cur.name, msg->name);
    strcpy(cur.data, "history");

    if (send(sd, &cur, sizeof(cur), 0) < 0) {
        perror("client send() history record!");
        exit(1);
    }

    memset(&rcvMsg, 0, sizeof(rcvMsg));
    if (recv(sd, &rcvMsg, sizeof(rcvMsg), 0) < 0) {
        perror("client recv() history record response.");
        exit(1);
    }

    printf("----------------------------------------------------------------------\n");
    printf("%s search word history record:\n%s\n", msg->name, rcvMsg.data);
    printf("----------------------------------------------------------------------\n");

    return;
}

/**
 * @brief Function to cancel an account
 * 
 * @param sd Socket descriptor
 * @return None
 * @note Deleting personal information from the server database
 */
static void do_logout(int sd)
{
    MSG cur, rcvMsg;

    printf("logout ...\n");
    printf("Please enter your username and password.\n");
    cur.type = htonl(O);
    scanf("%s", cur.name);
    scanf("%s", cur.data);

    if (send(sd, &cur, sizeof(cur), 0) < 0) {
        perror("client send() history record!");
        exit(1);
    }

    memset(&rcvMsg, 0, sizeof(rcvMsg));
    if (recv(sd, &rcvMsg, sizeof(rcvMsg), 0) < 0) {
        perror("client recv() history record response.");
        exit(1);
    }

    printf("user:%s and %s!\n", cur.name, rcvMsg.data);

    return;
}

/**
 * @brief The client main function is used to query word interpretation
 * 
 * @param argc Number of command line parameters
 * @param argv Strings on the command line
 * @return None
 * @note None
 */
int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server_addr;
    int menu1, menu2;
    int ret;
    MSG msg;

    // socket()
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    // connect()
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, SERVERIP, &server_addr.sin_addr);
    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(1);
    }

    while (1) {
        printf("----------------------------------------\n");
        printf("|                                      |\n");
        printf("|  1.register 2.login 3.quit 4.logout  |\n");
        printf("|                                      |\n");
        printf("----------------------------------------\n");
        printf("Please select a number:\n");
        scanf("%d", &menu1);
        // getchar();

        switch (menu1) {
            case 1:
                do_register(sd);
                break;
            case 2:
                ret = do_login(&msg, sd);   // msg:Get personal information
                if (!ret) {
                    printf("login ok!\n");
                    goto next;      // Jump after successful login
                }
                printf("Please reselect!\n");
                break;
            case 3:
                close(sd);
                exit(0);
                break;
            case 4:
                do_logout(sd);
                break;    
            default:
                printf("Please enter valid data!\n");
                break;
        }
    }

next:           // login ok!
    while (1) {
        printf("------------------------------------------\n");
        printf("|                                        |\n");
        printf("|  1.query word 2.history record 3.quit  |\n");
        printf("|                                        |\n");
        printf("------------------------------------------\n");
        printf("Please select a number:\n");
        scanf("%d", &menu2);
        // getchar();

        switch (menu2) {
            case 1:
                do_query(&msg, sd); // msg:save record (personal information)
                break;
            case 2:
                do_history(&msg, sd);
                break;
            case 3:
                close(sd);
                exit(0);
                break;
            default:
                printf("Please enter valid data!\n");
                break;
        }
    }
    
    exit(0);
}