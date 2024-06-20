#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#include <proto.h>

#define BUFSIZE         1024 * 2

/**
 * @brief Callback function, used to process query results; database callback function,
 * callback as many times as there are data in the database
 * 
 * @param data Passed parameters: ("sql_select")
 * @param argc Number of columns: ID,NAME,AGE,ADDRESS,SALARY (argc == 5)
 * @param argv Row Data: 'Paul', 32, 'California', 20000.00 (argv[0] == 'Paul')
 * @param azColName Column Fields: (azColName[0] == ID)
 * @return Returns 0 if successful
 * @note You must write return 0
 */
static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    printf("The callback function is called:data = %s\n", (char*)data);
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

/**
 * @brief Registering functions
 * 
 * @param msg Message
 * @param cd Client socket descriptor
 * @param db Database pointer
 * @return None
 * @note Insert the personal information sent by the customer into the database
 */
static void do_register(MSG* msg, int cd, sqlite3* db)
{
    MSG sendMsg;
    char sql_insert_buffer[BUFSIZE];
    int rc;
    char* zErrMsg = NULL;

    // printf("Client message text:(%d, %s, %s)\n", msg->type, msg->name, msg->data);
    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "insert into usr(name,password) "
                                                           "values('%s', '%s');", msg->name, msg->data);

    // sqlite3_exec(insert)
    rc = sqlite3_exec(db, sql_insert_buffer, callback, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        strcpy(sendMsg.data, "failure! User name already exist!");
        sqlite3_free(zErrMsg);
    } else {
        strcpy(sendMsg.data, "success!");
    }

    sendMsg.type = htonl(R);
    strcpy(sendMsg.name, "register");
    if (send(cd, &sendMsg, sizeof(sendMsg), 0) < 0) {
        perror("send() menu1");
        exit(1);
    }
    return;
}

/**
 * @brief Login functions
 * 
 * @param msg Message
 * @param cd Client socket descriptor
 * @param db Database pointer
 * @return None
 * @note Check the database to see if personal information exists
 * 
 * Name | Age
 * Alice| 43
 * Bob  | 28
 * Cindy| 21
 * 
 * column == 2, rows == 3 (valid data)
 * results[0] = "Name", results[1] = "Age", results[2] = "Alice",  results[3] = "43"
 * 
 */
static void do_login(MSG* msg, int cd, sqlite3* db)
{
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    MSG cur;
    char* name = msg->name;
    char* password = msg->data;
    char sql_insert_buffer[BUFSIZE];

    // Start verifying the personal information sent
    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "select * from usr where name = '%s' and password = '%s';", name, password);

    rc = sqlite3_get_table(db, sql_insert_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    if (nRows == 1) {   // The query is successful, and the user exists in the database
        strcpy(cur.data, "ok");
    } else {
        strcpy(cur.data, "usr/password error");
    }
    sqlite3_free_table(results);
    
    cur.type = htonl(L);
    strcpy(cur.name, "login");
    printf("server type:%d, name:%s, data:%s.\n", ntohl(cur.type), cur.name, cur.data);
    if (send(cd, &cur, sizeof(cur), 0) < 0) {
        perror("server send() person information");
        exit(1);
    }
    return;
}

/**
 * @brief Search word function
 * 
 * @param filename Dictionary path filename
 * @param desc Backfill the description of the word found
 * @param word Need to search for words
 * @return success 0; failure 1
 * @note Function to look up a word from a dictionary
 */
static int do_search_word(char* filename, MSG* msg, char* word)
{
    FILE *fp;
    char line[BUFSIZE];
    char* ret;
    char* token;
    int len;

    fp = fopen(filename, "r");
    if (NULL == fp) {
        fprintf(stderr, "fopen() failed! errno = %d\n", errno);
        return 1;
    }

    while (1) {
        ret = fgets(line, sizeof(line), fp);
        if (NULL == ret)
            break;
        token = strtok(line, ":");  // token is not a pointer to a string constant
        if (!strcmp(token, word)) {
            token = strtok(NULL, ":");
            size_t len = strlen(token);
            if (len > 0 && token[len - 1] == '\n') {    // Throw away the line breaks
                token[len - 1] = '\0';
            }
            strcpy(msg->data, token);
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return 1;
}

/**
 * @brief Query functions
 * 
 * @param msg Message
 * @param cd Client socket descriptor
 * @param db Database pointer
 * @return None
 * @note None
 */
static void do_query(MSG* msg, int cd, sqlite3* db)
{
    MSG cur;
    int ret = 0;
    char sql_insert_buffer[BUFSIZE];
    char word[64];  // query word
    char name[64];  // user name
    time_t current_time;
    char time_str[128];
    char* zErrMsg = NULL;
    int rc;

    strcpy(word, msg->data);
    strcpy(name, msg->name);
    cur.type = htonl(Q);

    ret = do_search_word(DISTIONARY_PATH, &cur, word);
    if (ret == 0) {
        // Find the word desc success
        printf("%s:%s.\n", word, cur.data);
        time(&current_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&current_time));

        // Insert the current time and username into the record database
        snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "insert into record(name,date,word) "
                                                               "values('%s', '%s', '%s');", name, time_str, word);

        rc = sqlite3_exec(db, sql_insert_buffer, callback, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            exit(1);
        } else {
            fprintf(stdout, "insert success!\n");
        }

        strcpy(cur.name, "ok");
    } else {
        strcpy(cur.name, "no");
    }

    if (send(cd, &cur, sizeof(cur), 0) < 0) {
        perror("server send() query word!");
        exit(1);
    }

    return;
}

/**
 * @brief History record functions
 * 
 * @param msg Message
 * @param cd Client socket descriptor
 * @param db Database pointer
 * @return None
 * @note None
 */
static void do_history(MSG* msg, int cd, sqlite3* db)
{
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    MSG cur;
    char* name = msg->name;
    char sql_insert_buffer[BUFSIZE];
    char temp[BUFSIZE];
    int offset = 0;

    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "select * from record where name = '%s';", name);

    rc = sqlite3_get_table(db, sql_insert_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    if (nRows != 0) {   // The query is successful, and the user exists in the database
        for (int row = 0; row <= nRows; row++) {
            for (int col = 0; col < nCols; col++) {
                offset += snprintf(temp + offset, sizeof(temp) - offset, "| %-20s ", results[row * nCols + col]);
            }
            offset += snprintf(temp + offset, sizeof(temp) - offset, "|\n");
        }
        printf("%s\n", temp);
        strncpy(cur.data, temp, strlen(temp) - 1);  // -1 Because the last one is a line break
    } else {
        strcpy(cur.data, "No query records");
    }
    sqlite3_free_table(results);
    
    cur.type = htonl(H);
    strcpy(cur.name, "history");
    if (send(cd, &cur, sizeof(cur), 0) < 0) {
        perror("server send() person information");
        exit(1);
    }
    
    return;
}

/**
 * @brief Function to cancel an account
 * 
 * @param msg Message
 * @param cd Client socket descriptor
 * @param db Database pointer
 * @return None
 * @note None
 */
static void do_logout(MSG* msg, int cd, sqlite3* db)
{
    MSG sendMsg;
    char sql_delete_usr_buffer[BUFSIZE];
    char sql_delete_record_buffer[BUFSIZE];
    int rc1, rc2;
    char* zErrMsg = NULL;
    int rows_deleted;

    snprintf(sql_delete_usr_buffer, sizeof(sql_delete_usr_buffer), "delete from usr where name = '%s';", msg->name);
    snprintf(sql_delete_record_buffer, sizeof(sql_delete_record_buffer), "delete from record where name = '%s';", msg->name);

    // sqlite3_exec(insert)
    rc1 = sqlite3_exec(db, sql_delete_usr_buffer, callback, NULL, &zErrMsg);
    if (rc1 != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else if (rc1 == SQLITE_OK) {
        rows_deleted = sqlite3_changes(db);
        if (rows_deleted > 0) {
            strcpy(sendMsg.data, "logout success");
            printf("Deletion successful. Rows deleted: %d\n", rows_deleted);
        } else {
            strcpy(sendMsg.data, "failure! User does not exist");
            printf("No matching rows found to delete.\n");
        }
    }

    rc2 = sqlite3_exec(db, sql_delete_record_buffer, callback, NULL, &zErrMsg);
    if (rc2 != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sendMsg.type = htonl(O);
    strcpy(sendMsg.name, "logout");
    if (send(cd, &sendMsg, sizeof(sendMsg), 0) < 0) {
        perror("send() logout");
        exit(1);
    }

    return;
}

/**
 * @brief Do work function
 * 
 * @param db Database pointer
 * @param cd Client socket descriptor
 * @return None
 * @note This work function is in a loop and keeps communicating with the client
 */
static void do_job(sqlite3* db, int cd)
{
    MSG readMsg;
    int n_read;

    // Continuously loop to receive client information and process it
    while (1) {
        memset(&readMsg, 0, sizeof(readMsg));
        n_read = read(cd, &readMsg, sizeof(readMsg));
        readMsg.type = ntohl(readMsg.type);  // Because it is 4 bytes, the network byte order needs to be changed
        if (n_read == -1) {
            perror("read()");
        } else if (n_read > 0) {
            printf("Client message num:%d, text:(%d, %s, %s)\n", n_read, readMsg.type, readMsg.name, readMsg.data);
        } else if (n_read == 0) {		// Disconnecting means reading data as 0
            printf("client quit!\n");
            break;
        }

        switch (readMsg.type) {
            case R:
                do_register(&readMsg, cd, db);
                break;
            case L:
                do_login(&readMsg, cd, db);
                break;
            case Q:
                do_query(&readMsg, cd, db);
                break;
            case H:
                do_history(&readMsg, cd, db);
                break;
            case O:
                do_logout(&readMsg, cd, db);
                break;
            default:
                printf("Invalid data msg.\n");
                break;
        }
    }
    
    return;
}

/**
 * @brief Server main function
 * 
 * @param argc Number of command line parameters
 * @param argv Strings on the command line
 * @return None
 * @note None
 */
int main(int argc, char *argv[])
{
    sqlite3* db;
    char* zErrMsg = NULL;
    int rc;
    int sd, cd;
    struct sockaddr_in local_addr, client_addr;
    socklen_t client_addr_len;
    pid_t pid;
    char ipstr[BUFSIZE];
    MSG readMsg;
    int n_read;

    // sqlite3_open()
    if ((rc = sqlite3_open(DATABASE, &db)) == SQLITE_OK) {
        printf("open %s success\n", DATABASE);
    } else {
        printf("error:%s, %d\n", sqlite3_errmsg(db), rc);
        if (rc == 14) {
            printf("can not open.\n");
        }
        return -1;
    }

    // sqlite3_exec(create table)
    // user table
    const char* sql_create_usr_table =   "create table usr("
                                         "name      text    primary key  not NULL,"
                                         "password  text    not NULL);";
    // int sqlite3_exec(sqlite3* db, const char* sql, int (*callback)(void*,int,char**,char**), void* data, char** errmsg);
    rc = sqlite3_exec(db, sql_create_usr_table, callback, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Usr table created successfully!\n");
    }
    // history word record table
    const char* sql_create_record_table =   "create table record("
                                            "name      text    not NULL,"
                                            "date      text    not NULL,"
                                            "word      text    not NULL);";
    rc = sqlite3_exec(db, sql_create_record_table, callback, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Record table created successfully!\n");
    }

    // socket()
    sd = socket(AF_INET, SOCK_STREAM, 0);       // IPPROTO_TCP, IPPRORO_SCTP
    if (sd < 0) {
        fprintf(stderr, "%d:socket()", __LINE__);
        exit(1);
    }

    // setsockopt()
    int val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) { // SO_REUSEADDR -- reuse
        perror("setsockopt");
        exit(1);
    }

    // bind()
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &local_addr.sin_addr);
    if (bind(sd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind()");
        exit(1);
    }

    // listen()
    if (listen(sd, 100)) {
        perror("listen()");
        exit(1);
    }

    // Dealing with zombie processes
    signal(SIGCHLD, SIG_IGN);

    // accept()
    client_addr_len = sizeof(client_addr);
    while (1) {
        cd = accept(sd, (struct sockaddr *)&client_addr, &client_addr_len); // Store the client's address information while accepting the connection
        if (cd < 0) {
            perror("accept()");
            exit(1);
        }

        // fork()
        pid = fork();
        if (pid < 0) {
            perror("fork()");
            exit(1);
        }
        if (pid == 0) { // The child process works, and the parent process continues to accept and wait for the connection
            close(sd);
            inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, BUFSIZE);
            printf("Client ip:%s and port:%d.\n", ipstr, ntohs(client_addr.sin_port));
            
            // do_job()
            do_job(db, cd); // Functions that do work for the child process

            close(cd);
            exit(0);
        }
        if (pid > 0) {  // The parent process continues to accept the next client
            close(cd);
        }
    }

    close(sd);
    sqlite3_close(db);

    exit(0);
}