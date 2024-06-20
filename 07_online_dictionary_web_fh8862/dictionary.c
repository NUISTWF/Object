#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define SIZE                128
#define BUFSIZE             1024 * 2
#define NAMESIZE            32
#define DATASIZE            1024
#define SERVERIP            "192.168.1.3"
#define SERVERPORT          "114"
#define DATABASE            "dictionary.db"
#define DISTIONARY_PATH     "dictionary.txt"

enum {
    R = 1,                  // R: user - register
    L,                      // L: user - login
    Q,                      // Q: user - query
    H,                      // H: user - history
    O                       // O: user - logout
};

static void do_register(char* username, char* password, sqlite3* db)
{
    char sql_insert_buffer[BUFSIZE];
    int rc;
    char* zErrMsg = NULL;

    // printf("Client message text:(%d, %s, %s)\n", msg->type, msg->name, msg->data);
    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "insert into usr(name,password) "
                                                           "values('%s', '%s');", username, password);

    rc = sqlite3_exec(db, sql_insert_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        printf("error in register");
        sqlite3_free(zErrMsg);
    } else {
        printf("register success!");
    }

    return;
}

static void do_login(char* username, char* password, sqlite3* db)
{
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    char sql_insert_buffer[BUFSIZE];

    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "select * from usr where name = '%s' and password = '%s';", username, password);

    sqlite3_get_table(db, sql_insert_buffer, &results, &nRows, &nCols, &zErrMsg);

    if (nRows == 1) {   // The query is successful, and the user exists in the database
        printf("login ok!");
    } else {
        printf("usr or password error!");
    }
    sqlite3_free_table(results);

    return;
}

static int do_search_word(char* filename, char* msg, char* word)
{
    FILE *fp;
    char line[BUFSIZE];
    char* ret;
    char* token;
    int len;

    fp = fopen(filename, "r");

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
            strcpy(msg, token);
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return 1;
}

static void do_query(char* username, char* word, sqlite3* db)
{
    int ret = 0;
    char sql_insert_buffer[BUFSIZE];
    time_t current_time;
    char time_str[128];
    char* zErrMsg = NULL;
    char cur[BUFSIZE];

    ret = do_search_word(DISTIONARY_PATH, cur, word);
    if (ret == 0) {
        // Find the word desc success
        printf("%s:%s.", word, cur);
        time(&current_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&current_time));

        // Insert the current time and username into the record database
        snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "insert into record(name,date,word) "
                                                               "values('%s', '%s', '%s');", username, time_str, word);

        sqlite3_exec(db, sql_insert_buffer, NULL, NULL, &zErrMsg);
    } else {
        printf("No such word");
    }

    return;
}

static void do_logout(char* username, char* password, sqlite3* db)
{
    char sql_delete_usr_buffer[BUFSIZE];
    char sql_delete_record_buffer[BUFSIZE];
    int rc;
    char* zErrMsg = NULL;
    int rows_deleted;

    snprintf(sql_delete_usr_buffer, sizeof(sql_delete_usr_buffer), "delete from usr where name = '%s';", username);
    snprintf(sql_delete_record_buffer, sizeof(sql_delete_record_buffer), "delete from record where name = '%s';", username);

    rc = sqlite3_exec(db, sql_delete_usr_buffer, NULL, NULL, &zErrMsg);
    if (rc == SQLITE_OK) {
        rows_deleted = sqlite3_changes(db);
        if (rows_deleted > 0) {
            printf("logout success");
        } else {
            printf("No this user.\n");
        }
    }

    sqlite3_exec(db, sql_delete_record_buffer, NULL, NULL, &zErrMsg);

    return;
}

static void do_history(char* username, char* password, sqlite3* db)
{
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    char sql_insert_buffer[BUFSIZE];
    char temp[BUFSIZE];
    int offset = 0;

    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "select * from record where name = '%s';", username);
    sqlite3_get_table(db, sql_insert_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (nRows != 0) {   // The query is successful, and the user exists in the database
        for (int row = 0; row <= nRows; row++) {
            for (int col = 0; col < nCols; col++) {
                offset += snprintf(temp + offset, sizeof(temp) - offset, "| %-20s ", results[row * nCols + col]);
            }
            offset += snprintf(temp + offset, sizeof(temp) - offset, "|\n");
        }
        printf("%s\n", temp);
    } else {
        printf("No such record!");
    }
    sqlite3_free_table(results);
    
    return;
}

static int do_job(sqlite3* db)
{
    char *dataLen = NULL;
    char buffer[100] = {0};
    int len = 0;
    char* token;
    char username[SIZE];
    char password[SIZE];
    char word[SIZE];
    char type[SIZE];

    dataLen = getenv("CONTENT_LENGTH"); // 获取文本内容长度
    if (NULL == dataLen) {
        printf("error1");
        return 0;
    } else {
        len = atoi(dataLen);    // 先通过getenv获取长度再通过fgets和scanf一样的效果，获取内容
        if (len > 0) {
            if (NULL == fgets(buffer, len + 1, stdin)) { // buffer == "xm-123-impute-register"
                printf("error2");
                return 0;
            } else {
                token = strtok(buffer, "-");
                strcpy(username, token);

                token = strtok(NULL, "-");
                strcpy(password, token);

                token = strtok(NULL, "-");
                strcpy(word, token);

                token = strtok(NULL, "-");
                strcpy(type, token);

                if (!strcmp(type, "register")) {
                    do_register(username, password, db);
                } else if (!strcmp(type, "login")) {
                    do_login(username, password, db);
                } else if (!strcmp(type, "query")) {
                    do_query(username, word, db);
                } else if (!strcmp(type, "logout")) {
                    do_logout(username, password, db);
                } else if (!strcmp(type, "history")) {
                    do_history(username, password, db);
                }
            }
        }
    }

    return 0;
}


int main()
{
    sqlite3* db;
    char* zErrMsg = NULL;
    int n_read;

    printf("Content-type:text/html\n\n"); // 这句一定要加上

    sqlite3_open(DATABASE, &db);

    const char* sql_create_usr_table =   "create table usr("
                                         "name      text    primary key  not NULL,"
                                         "password  text    not NULL);";
    sqlite3_exec(db, sql_create_usr_table, NULL, NULL, &zErrMsg);

    const char* sql_create_record_table =   "create table record("
                                            "name      text    not NULL,"
                                            "date      text    not NULL,"
                                            "word      text    not NULL);";
    sqlite3_exec(db, sql_create_record_table, NULL, NULL, &zErrMsg);

    int ret = do_job(db);

    sqlite3_close(db);

    return 0;
}