// 服务器代码

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/syscall.h>
#include <mqueue.h>
#include <errno.h>
#include <sys/wait.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>

#include <proto.h>

static int client_sockets[MAX_CLIENTS];
static sqlite3* db;
static int mark = 0;

// 去掉尾巴的换行符'\n'
static void remove_newline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

// 这是服务器可以给任意客户端发信息（只需要指定ip和port即可）
// 因为名字是唯一的，也可以指定名字来发给客户，更接近真实情况，服务器不可访问用户信息，采用ip+port方式发信息
static void *send_client(void *arg)
{
    char ip[BUFFER_SIZE] = {0};
    char port[BUFFER_SIZE] = {0};
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int i = 0;
    MSG msg;
    char client_ip[BUFFER_SIZE] = {0};
    char client_port[BUFFER_SIZE] = {0};
    size_t len = 0;

    while (1)
    {
        printf("input ip, port and message: ");
        fgets(ip, sizeof(ip) - 1, stdin);
        fgets(port, sizeof(port) - 1, stdin);
        fgets(msg.data, sizeof(msg) - 1, stdin);

        remove_newline(ip);
        remove_newline(port);
        remove_newline(msg.data);

        msg.type = htonl(S);
        strcpy(msg.name, "server");

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            getpeername(client_sockets[i], (struct sockaddr *)&address, &addrlen);
            strcpy(client_ip, inet_ntoa(address.sin_addr));
            snprintf(client_port, sizeof(client_port), "%d", ntohs(address.sin_port));
            if ((strcmp(client_ip, ip) == 0) && (strcmp(client_port, port) == 0))
            {
                break;
            }
        }

        write(client_sockets[i], &msg, sizeof(msg));
    }
}

// 注册函数
static void do_register(MSG* msg, int client_socket, sqlite3* db)
{
    MSG sendMsg;
    char sql_insert_buffer[SQL_BUFFER_SIZE];
    int rc;
    char* zErrMsg = NULL;
    char ip[BUFFER_SIZE];
    char port[BUFFER_SIZE];
    char state[BUFFER_SIZE];

    strcpy(ip, "0.0.0.0");
    strcpy(port, "9999");   // 注册的同时一并赋上初值（初值任意即可，因为每次用户登录都会更新ip和端口）
    strcpy(state, "0");

    // printf("Client message text:(%d, %s, %s)\n", msg->type, msg->name, msg->data); // 测试语句
    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "insert into usr(name,ip,port,state,password) "
                                                           "values('%s', '%s', '%s', '%s', '%s');", msg->name, ip, port, state, msg->data);
    rc = sqlite3_exec(db, sql_insert_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        strcpy(sendMsg.data, "failure! User name already exist!");
        sqlite3_free(zErrMsg);
    }
    else
    {
        strcpy(sendMsg.data, "success!");
    }

    sendMsg.type = htonl(R);
    strcpy(sendMsg.name, "register");
    if (send(client_socket, &sendMsg, sizeof(sendMsg), 0) < 0)
    {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    return ;
}

// 登录模块还需要做一件事，就是登陆后检测数据库中是否有notsend信息，有的话现需要将数据发给刚登陆的用户
// 每次检测到notsend数据记录发送完后要把notsend改为history，不然每次登录都会发，然后重复收到数据
static void do_login(MSG* msg, int client_socket, sqlite3* db, char* ip, char* port)
{
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    MSG cur, sendMsg;
    char* name = msg->name;     // 登陆的用户名字
    char* password = msg->data; // 登陆的用户密码
    char sql_insert_buffer[SQL_BUFFER_SIZE];
    char sql_chat_buffer[SQL_BUFFER_SIZE];
    char sql_insert_ip_port_buffer[SQL_BUFFER_SIZE];
    char client_ip[BUFFER_SIZE] = {0};
    char client_port[BUFFER_SIZE] = {0};
    char sql_update_buffer[BUFFER_SIZE] = {0};
    char state[BUFFER_SIZE];
    int row = 0;
    char send_file_name[BUFFER_SIZE] = {0};
    FILE* file;
    int bytesRead = 0;
    int offset = 0;

    strcpy(client_ip, ip);
    strcpy(client_port, port);
    strcpy(state, "1");  // 登陆成功状态改为1，代表在线

    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "select * from usr where name = '%s' and password = '%s';", name, password);

    rc = sqlite3_get_table(db, sql_insert_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    if (nRows == 1)
    {
        // 登录成功后，将当前用户所在地的ip和端口也一并保存在数据库中，这样每次登录就会更新数据库
        snprintf(sql_insert_ip_port_buffer, sizeof(sql_insert_ip_port_buffer), "update usr set ip = '%s', port = '%s', state = '%s' where name = '%s';", ip, port, state, name);
        rc = sqlite3_exec(db, sql_insert_ip_port_buffer, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            strcpy(cur.data, "failure! update ip and port error!");
            sqlite3_free(zErrMsg);
        }
        else
        {
            strcpy(cur.data, "ok");
        }
    }
    else
    {
        strcpy(cur.data, "usr or password error");
    }
    sqlite3_free_table(results);
    
    cur.type = htonl(L);
    strcpy(cur.name, "login");
    // printf("server type:%d, name:%s, data:%s.\n", ntohl(cur.type), cur.name, cur.data); // 测试语句
    if (send(client_socket, &cur, sizeof(cur), 0) < 0)
    {
        perror("server send() person information");
        exit(EXIT_FAILURE);
    }

    // 登陆成功且该用户地址信息和状态保存成功后，查看有没有该用户的滞留信息
    // 如果有滞留信息，则将滞留信息发送给该用户（只查看friendname是自己的名字的，也就代表发给自己的）
    // 如果没有滞留信息，则跳过
    /* 
    例如，筛选出来的表如下：
        myname | friendname | date      | state   | msgrecord  | type
        Alice  | Cindy      | 24.07.09  | notsend | "hello"    | msg
        Bob    | Cindy      | 24.07.11  | notsend | "good!"    | msg
        Bob    | Cindy      | 24.07.12  | notsend | "nice!"    | msg
        Alice  | Cindy      | 24.07.09  | notsend | "word.txt" | file
    此时 nRows = 3, nCols = 5 
    */
    snprintf(sql_chat_buffer, sizeof(sql_chat_buffer), "select * from record where friendname = '%s' and state = '%s';", name, "notsend");
    rc = sqlite3_get_table(db, sql_chat_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    if (nRows == 0)
    {
        // printf("this client has no message\n");
    }
    if (nRows != 0)
    {
        // 需要循环获取发送消息（注意加上是谁发给我的什么消息，当前登录的客户要知道谁给他发的什么消息）
        for (row = 1; row <= nRows ; row++)
        {
            if (strcmp(results[(row + 1) * nCols - 1], "msg") == 0) // 判断是不是msg消息还是文件，所发内容是不同的
            {
                memset(&sendMsg, 0, sizeof(sendMsg));
                sendMsg.type = htonl(S);
                strcpy(sendMsg.name, results[row * nCols]);
                strcpy(sendMsg.data, results[row * nCols + 4]);
                // printf("server type:%d, name:%s, data:%s.\n", ntohl(sendMsg.type), sendMsg.name, sendMsg.data);
                if (send(client_socket, &sendMsg, sizeof(sendMsg), 0) < 0)
                {
                    perror("send failed in login notsend");
                    exit(EXIT_FAILURE);
                }
            }
            // 下面是检测到该消息是文件类型的话就转发本地服务器上的文件给该客户
            if (strcmp(results[(row + 1) * nCols - 1], "file") == 0)
            {
                memset(&sendMsg, 0, sizeof(sendMsg));
                // printf("current file type:%s\n", results[(row + 1) * nCols - 1]); // 调试语句
                sendMsg.type = htonl(F);
                // sendMsg.name 的格式应该是: xiaowang#word.txt （名字加文件名）
                strcpy(sendMsg.name, results[row * nCols]); // 发送人保存到sendMsg.name中
                offset = strlen(sendMsg.name);
                strcpy(sendMsg.name + offset, "#");
                strcpy(send_file_name, results[row * nCols + 4]); // send_file_name是要发送的文件名字，然后去打开文件发送内容即可
                strcpy(sendMsg.name + offset + 1, send_file_name);

                // 往服务端发送数据（将文件打开，内容发过去，文件会保存在服务器当前界面的目录下）
                file = fopen(send_file_name, "r");  // 打开需要发送的文件
                if (!file)
                {
                    printf("in server, file name error!\n");
                    continue;
                }
                // printf("file name:%s\n", send_file_name); // 调试语句
                // 发送文件内容（读取到 sendMsg.data 再发送）
                while ((bytesRead = fread(sendMsg.data, sizeof(char), BUFFER_SIZE, file)) > 0)
                {
                    // printf("text:%s\n", sendMsg.data); // 调试语句
                    if (send(client_socket, &sendMsg, sizeof(sendMsg) - sizeof(sendMsg.data) + bytesRead, 0) < 0)  // 往服务端发送数据
                    {
                        printf("server send() file to friend error!");
                        exit(EXIT_FAILURE);
                    }
                }

                fclose(file);
            }
        }
    }
    sqlite3_free_table(results);

    // 循环发完消息，就该修改所有的本用户的notsend了（一定只能修改本用户的记录，其他人的不能修改） notsend->history
    snprintf(sql_update_buffer, sizeof(sql_update_buffer), "update record set state = '%s' where friendname = '%s';", "history", name);
    
    rc = sqlite3_exec(db, sql_update_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    
    return ;
}

// 注销函数（首先验证密码是否正确，再删除所有包含该用户名的信息，三个表相关的都删除）
static void do_logout(MSG* msg, int client_fd, sqlite3* db)
{
    MSG sendMsg;
    char usrname[SQL_BUFFER_SIZE];
    char password[SQL_BUFFER_SIZE];
    char sql_usr_password_buffer[SQL_BUFFER_SIZE];
    char sql_logout_usr_buffer[SQL_BUFFER_SIZE];
    char sql_chat_record_buffer[SQL_BUFFER_SIZE];
    char sql_friendlist_buffer[SQL_BUFFER_SIZE];
    
    int rc;
    char* zErrMsg = NULL;
    int rows_deleted;
    char **results;
    int nRows, nCols;

    strcpy(usrname, msg->name);     // 获取到需要删除的用户名和密码
    strcpy(password, msg->data);    
    
    // 验证密码是否正确
    snprintf(sql_usr_password_buffer, sizeof(sql_usr_password_buffer), "select * from usr where name = '%s' and password = '%s';", usrname, password);
    rc = sqlite3_get_table(db, sql_usr_password_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    if (nRows == 1)
    {
        // 如果密码正确，则删除该用户的所有信息（先删除usr表中的信息）
        snprintf(sql_logout_usr_buffer, sizeof(sql_logout_usr_buffer), "delete from usr where name = '%s';", usrname);
        rc = sqlite3_exec(db, sql_logout_usr_buffer, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else if (rc == SQLITE_OK)
        {
            rows_deleted = sqlite3_changes(db);
            if (rows_deleted > 0)
            {
                // 删完usr表再删record表
                snprintf(sql_chat_record_buffer, sizeof(sql_chat_record_buffer), "delete from record where (myname = '%s') or (friendname = '%s');", usrname, usrname);
                sqlite3_exec(db, sql_chat_record_buffer, NULL, NULL, &zErrMsg);
                // 删完record表再删friendlist表
                snprintf(sql_friendlist_buffer, sizeof(sql_friendlist_buffer), "delete from friendlist where (myname = '%s') or (friendname = '%s');", usrname, usrname);
                sqlite3_exec(db, sql_friendlist_buffer, NULL, NULL, &zErrMsg);
                
                // 删除成功，发送成功消息给客户端
                strcpy(sendMsg.data, "logout success");
                // printf("Deletion successful. Rows deleted: %d\n", rows_deleted);
            }
            else
            {
                strcpy(sendMsg.data, "failure! User does not exist");
                printf("No matching rows found to delete.\n");
            }
        }
    }
    else
    {
        strcpy(sendMsg.data, "usr or password error");
    }
    sqlite3_free_table(results);

    sendMsg.type = htonl(O);
    strcpy(sendMsg.name, usrname);
    if (send(client_fd, &sendMsg, sizeof(sendMsg), 0) < 0)
    {
        perror("send() logout");
        exit(EXIT_FAILURE);
    }

    return;
}

// 添加好友函数（将拿到的usr用户名字去usr表找一下，找到的话就加入到friendlist的表中，找不到就回复没有此用户）
static void do_add_friend(MSG* msg, int client_socket, sqlite3* db)
{
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    MSG cur;
    char* friendname = msg->name;  // 好友名字
    char* myname = msg->data;      // 自己的名字
    char sql_add_friend_buffer[SQL_BUFFER_SIZE];
    char sql_friendlist_buffer[SQL_BUFFER_SIZE];

    // Start verifying the personal information sent
    snprintf(sql_add_friend_buffer, sizeof(sql_add_friend_buffer), "select * from usr where name = '%s';", friendname);

    rc = sqlite3_get_table(db, sql_add_friend_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    if (nRows == 1)     // The query is successful, and the user exists in the database
    {
        // 这里将好友信息插入到friendlist表中
        snprintf(sql_friendlist_buffer, sizeof(sql_friendlist_buffer), "insert into friendlist(myname,friendname) "
                                                                       "values('%s', '%s');", myname, friendname);
        // sqlite3_exec(insert)
        rc = sqlite3_exec(db, sql_friendlist_buffer, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            strcpy(cur.data, "failure! User name already exist!");
            sqlite3_free(zErrMsg);
        }
        else
        {
            strcpy(cur.data, "success");    // 插入成功就发送消息成功了
        }
    }
    else
    {
        strcpy(cur.data, "error [this usr not exist]");
    }
    sqlite3_free_table(results);
    
    cur.type = htonl(A);
    strcpy(cur.name, "add friend");
    // printf("server type:%d, name:%s, data:%s.\n", ntohl(cur.type), cur.name, cur.data); // 测试语句
    if (send(client_socket, &cur, sizeof(cur), 0) < 0)
    {
        perror("server send() person information");
        exit(EXIT_FAILURE);
    }

    return ;
}

// 查询好友列表
/*
 * myname | friendname
 * xiaomo | Alice
 * xiaomo | Bob
 * xiaomo | Cindy
 * 
 * column == 2, rows == 3 (valid data)
 * results[0] = "myname", results[1] = "friendname", results[2] = "xiaomo",  results[3] = "Alice"
 * 
 */
static void do_query_friendlist(MSG* msg, int client_socket, sqlite3* db)
{
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    MSG cur;
    int row;
    char temp[BUFFER_SIZE];
    int offset = 0;
    char* myname = msg->name;  // 好友名字
    char sql_query_friendlist_buffer[SQL_BUFFER_SIZE];
    char sql_query_state_buffer[SQL_BUFFER_SIZE];
    
    memset(&cur, 0, sizeof(cur));

    // 这边是查询好友的数据库friendlist表
    snprintf(sql_query_friendlist_buffer, sizeof(sql_query_friendlist_buffer), "select * from friendlist where myname = '%s';", myname);
    rc = sqlite3_get_table(db, sql_query_friendlist_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } // 找到所有的名字是myname的记录

    if (nRows != 0)   // The query is successful, and the user exists in the database
    {
        for (row = 1; row <= nRows; row++)
        {
            offset += snprintf(temp + offset, sizeof(temp) - offset, "%s\n", results[row * nCols + 1]);
        }
        // printf("%s", temp);
        strncpy(cur.data, temp, strlen(temp) - 1);  // -1 Because the last one is a line break
    }
    else
    {
        strcpy(cur.data, "You don't have friends yet, go ahead and add your friends");
    }
    sqlite3_free_table(results);

    cur.type = htonl(Q);
    strcpy(cur.name, "query friendlist");
    // printf("%s", cur.data); // 发送前打印下发送的数据，调试语句
    if (send(client_socket, &cur, sizeof(cur), 0) < 0)
    {
        perror("server send() person information");
        exit(EXIT_FAILURE);
    }

    return ;
}

// 删除聊天记录（msg包含了自己的名字和需要操作的好友名字）
static void do_delete_chat_record(MSG* msg, int client_socket, sqlite3* db)
{
    char myname[BUFFER_SIZE] = {0};
    char friendname[BUFFER_SIZE] = {0};
    char sql_chat_record_buffer[BUFFER_SIZE] = {0};
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    int row = 0;
    MSG sendMsg;
    int rows_deleted = 0;

    strcpy(myname, msg->name);
    strcpy(friendname, msg->data);

    // 删除记录是删除我发的和对方发我的【删除双方的聊天记录，不然每个用户需要一张聊天记录表，也可以这么做但没必要】
    snprintf(sql_chat_record_buffer, sizeof(sql_chat_record_buffer), "delete from record where (myname = '%s' and friendname = '%s') or (myname = '%s' and friendname = '%s');", myname, friendname, friendname, myname);
    rc = sqlite3_exec(db, sql_chat_record_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (rc == SQLITE_OK)
    {
        rows_deleted = sqlite3_changes(db);
        if (rows_deleted > 0)
        {
            strcpy(sendMsg.data, "success");
            // printf("Deletion chat record successful. Rows deleted: %d\n", rows_deleted);
        }
        else
        {
            strcpy(sendMsg.data, "failure! not chat record");
            // printf("No matching rows found to delete.\n");
        }
    }

    sendMsg.type = htonl(T);
    strcpy(sendMsg.name, "delete chat record");
    if (send(client_socket, &sendMsg, sizeof(sendMsg), 0) < 0)
    {
        perror("send failed in login notsend");
        exit(EXIT_FAILURE);
    }

    return ;
}

// 删除好友（删除好友需要删除所有的，包括聊天记录，以及好友列表中的信息）
static void do_delete_friend(MSG* msg, int client_socket, sqlite3* db)
{
    int rc;
    char* zErrMsg = NULL;
    MSG sendMsg;
    int rows_deleted;
    char* myname = msg->name;        // 自己名字
    char* friendname = msg->data;    // 要被删除的好友名字
    char sql_delete_friend_buffer[SQL_BUFFER_SIZE];

    snprintf(sql_delete_friend_buffer, sizeof(sql_delete_friend_buffer), "delete from friendlist where myname = '%s' and friendname = '%s';", myname, friendname);

    rc = sqlite3_exec(db, sql_delete_friend_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL delete error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (rc == SQLITE_OK)
    {
        rows_deleted = sqlite3_changes(db);
        if (rows_deleted > 0)
        {
            snprintf(sendMsg.data, sizeof(sendMsg.data), "delete your friend (%s) success", friendname);
            // printf("Deletion successful. usr:%s deleted: %d\n", friendname, rows_deleted);
        }
        else
        {
            strcpy(sendMsg.data, "failure! You don't have friends");
            // printf("No matching rows found to delete.\n");
        }
    }

    // 上面删除了好友列表信息，这边可以直接调用删除聊天记录的函数即可删除聊天记录
    do_delete_chat_record(msg, client_socket, db);

    sendMsg.type = htonl(D);
    strcpy(sendMsg.name, "delete friend");
    if (send(client_socket, &sendMsg, sizeof(sendMsg), 0) < 0)
    {
        perror("send() delete");
        exit(EXIT_FAILURE);
    }

    return ;
}

// 给好友发信息或者文件（服务器是中转用的，怎么挂钩用户和ip端口，每次登录都更新数据库的当前登录地址信息）（也需要关注好友是否离线，如果离线则将数据保存在当前服务器，等到对方一上线就自动转发）
/*
 * name  | ip           | port  | state | password
 * Alice | 192.168.1.1  | 12345 | 0     | 123
 * Bob   | 192.168.1.2  | 12346 | 0     | 456
 * Cindy | 192.168.1.3  | 12347 | 1     | 789
 * 
 * column == 3, rows == 4 (valid data)
 * results[0] = "name", results[1] = "ip", results[2] = "port",  results[3] = "password"
 * results[4] = "Alice", results[5] = "192.168.1.1", results[6] = "12345",  results[7] = "123"
 */
static void do_msg_transit(MSG* msg, int client_socket, sqlite3* db, char* my_sendname)
{
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char client_ip[BUFFER_SIZE] = {0};
    char client_port[BUFFER_SIZE] = {0};
    char ip[BUFFER_SIZE] = {0};
    char port[BUFFER_SIZE] = {0};
    int i;    
    char sendname[BUFFER_SIZE] = {0};  // 要发的客户名字
    char datamsg[BUFFER_SIZE] = {0};      // 要发的信息
    char sql_insert_buffer[BUFFER_SIZE] = {0};
    char sql_msg_buffer[BUFFER_SIZE] = {0};
    MSG cur;
    char mysendname[BUFFER_SIZE] = {0};
    time_t current_time;
    char time_str[BUFFER_SIZE] = {0};
    char state[BUFFER_SIZE] = {0};
    char history[BUFFER_SIZE] = {0};

    strcpy(sendname, msg->name);
    strcpy(datamsg, msg->data);
    strcpy(mysendname, my_sendname);

    // 查询数据库usr，找到对上名字的ip和端口，往这个ip端口发即可
    // 需要判断对方是否在线，在线的话就发送，不在线就保存在记录中，到时候对方登录的瞬间会自动发送
    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "select * from usr where name = '%s';", sendname);

    rc = sqlite3_get_table(db, sql_insert_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    if (nRows == 1) // 等于1代表找到了该用户
    {
        snprintf(client_ip, sizeof(client_ip), "%s", results[nCols + 1]);
        snprintf(client_port, sizeof(client_port), "%s", results[nCols + 2]);
        snprintf(state, sizeof(state), "%s", results[nCols + 3]);
        // printf("ip:%s, port:%s and state:%s\n", client_ip, client_port, state);
    }
    else
    {
        printf("usr ip and port error\n");
    }
    sqlite3_free_table(results);

    // 找到对应的客户端fd
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        getpeername(client_sockets[i], (struct sockaddr *)&address, &addrlen);
        strcpy(ip, inet_ntoa(address.sin_addr));
        snprintf(port, sizeof(port), "%d", ntohs(address.sin_port));
        if ((strcmp(client_ip, ip) == 0) && (strcmp(client_port, port) == 0))
        {
            break;
        }
    }

    if (strcmp(state, "1") == 0)
    {
        strcpy(history, "history");
    }
    else if (strcmp(state, "0") == 0)
    {
        strcpy(history, "notsend");
    }

    if (strcmp(state, "1") == 0)      // 如果在线就立即发送
    {
        // 此时这个 client_sockets[i] 就是客户端的socket
        strcpy(cur.name, mysendname); // 发送人的姓名
        strcpy(cur.data, datamsg);    // 发送的数据
        cur.type = htonl(S);          // S代表是发送的消息，不是交互
        if (send(client_sockets[i], &cur, sizeof(cur), 0) < 0)  // 发给对应的好友
        {
            perror("send() friend msg");
            exit(EXIT_FAILURE);
        }
    }

    // 下面不管在不在线都保存记录，只是保存的记录的state不一样
    time(&current_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&current_time));

    // 发送消息成功后，将消息保存到数据库的历史记录中（需要保存发送人和被发送人还有时间，状态，记录；此处记录区分notsend还是history，因为已经发送了） mysendname, sendname 分别是我发给对方，双方的名字
    snprintf(sql_msg_buffer, sizeof(sql_msg_buffer), "insert into record(myname,friendname,date,state,msgrecord,type) "
                                                     "values('%s', '%s', '%s', '%s', '%s', '%s');", mysendname, sendname, time_str, history, datamsg, "msg");
    rc = sqlite3_exec(db, sql_msg_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stdout, "insert record success!\n");
    }

    return ;
}

// 收到文件就将文件保存在本地，且聊天记录就记录为文件名字即可
static void do_file_transit(MSG* msg, int client_socket, sqlite3* db, char* my_sendname, int valread)  // my_sendname 是要发送的目标好友
{
    char* author;       // 发送人的名字
    FILE* localFile;    // 存储为服务器的文件描述符指针
    char filename[BUFFER_SIZE]; // 文件名
    MSG cur;
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char client_ip[BUFFER_SIZE] = {0};
    char client_port[BUFFER_SIZE] = {0};
    char ip[BUFFER_SIZE] = {0};
    char port[BUFFER_SIZE] = {0};
    int i;    
    char sendname[BUFFER_SIZE] = {0};  // 要发的客户名字
    char datamsg[BUFFER_SIZE] = {0};      // 要发的信息
    char sql_insert_buffer[BUFFER_SIZE] = {0};
    char sql_msg_buffer[BUFFER_SIZE] = {0};
    char mysendname[BUFFER_SIZE] = {0};     // 我自己的名字（发送者的名字）
    time_t current_time;
    char time_str[BUFFER_SIZE] = {0};
    char state[BUFFER_SIZE] = {0};
    char history[BUFFER_SIZE] = {0};
    int offset = 0;
    int valwrite = 0;

    // msg->name == xiaoming#file.txt
    strcpy(datamsg, msg->data);
    strcpy(mysendname, my_sendname);

    // 先保存文件内容到本地
    author = strtok(msg->name, "#");    // author 是发送的目标对象
    strcpy(filename, strtok(NULL, "#"));
    // printf("server your friend:[%s] --> file:[%s]\n", author, filename);
    localFile = fopen(filename, "w");
    if (localFile == NULL)
    {
        printf("ERROR opening local file\n");
        return ;
    }

    // 写入文件内容
    // printf("recv msg bytes:%d\n", valread); // 收到的包内文本内容是固定字节数
    valwrite = fwrite(msg->data, sizeof(char), valread - sizeof(cur.type) - sizeof(cur.name), localFile);
    // printf("write %d bytes\n", valwrite);
    fclose(localFile);
    // 此时文件应该保存在本地服务器了

    strcpy(sendname, author);

    // 查询数据库usr，找到对上名字的ip和端口，往这个ip端口发即可
    // 需要判断对方是否在线，在线的话就发送，不在线就保存在记录中，到时候对方登录的瞬间会自动发送
    snprintf(sql_insert_buffer, sizeof(sql_insert_buffer), "select * from usr where name = '%s';", sendname);

    rc = sqlite3_get_table(db, sql_insert_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    if (nRows == 1) // 等于1代表找到了该用户
    {
        snprintf(client_ip, sizeof(client_ip), "%s", results[nCols + 1]);
        snprintf(client_port, sizeof(client_port), "%s", results[nCols + 2]);
        snprintf(state, sizeof(state), "%s", results[nCols + 3]);
        printf("ip:%s, port:%s and state:%s\n", client_ip, client_port, state);
    }
    else
    {
        // printf("usr ip and port error\n");
    }
    sqlite3_free_table(results);

    // 找到对应的客户端fd
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        getpeername(client_sockets[i], (struct sockaddr *)&address, &addrlen);
        strcpy(ip, inet_ntoa(address.sin_addr));
        snprintf(port, sizeof(port), "%d", ntohs(address.sin_port));
        if ((strcmp(client_ip, ip) == 0) && (strcmp(client_port, port) == 0))
        {
            break;
        }
    }
    // printf("client fd:%d, ip:%s, port:%s\n", client_sockets[i], ip, port);

    if (strcmp(state, "1") == 0)
        strcpy(history, "history");
    else if (strcmp(state, "0") == 0)
        strcpy(history, "notsend");

    if (strcmp(state, "1") == 0)      // 如果在线就立即发送
    {
        // 此时这个 client_sockets[i] 就是客户端的socket
        // 发送人的姓名和文件名字保存在 cur.name 中
        offset = strlen(mysendname);
        strcpy(cur.name, mysendname);
        strcpy(cur.name + offset, "#");  // 在好友名字后面加上#，方便服务端判断
        strcpy(cur.name + offset + 1, filename);  // 在#后面加上文件名字
        strcpy(cur.data, datamsg);    // 需要发送的数据
        cur.type = htonl(F);          // F代表是发送的文件，不是交互
        // 这样发送的文件就是：F + name#filename + file data
        if (send(client_sockets[i], &cur, sizeof(cur) - sizeof(cur.data) + valwrite, 0) < 0)  // 发给对应的好友
        {
            perror("send() friend file");
            exit(EXIT_FAILURE);
        }
        // printf("send file success, accept name and file name:%s, text:%s\n", cur.name ,cur.data);
    }

    // 如果不在线就保存notsend的历史记录，等好友上线的时候自动将服务器保存的该文件发送过去
    // 下面不管在不在线都保存记录，只是保存的记录的state不一样
    time(&current_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&current_time));

    // 发送消息成功后，将消息保存到数据库的历史记录中（需要保存发送人和被发送人还有时间，状态，记录；此处记录区分notsend还是history，因为已经发送了） mysendname, sendname 分别是我发给对方，双方的名字
    snprintf(sql_msg_buffer, sizeof(sql_msg_buffer), "insert into record(myname,friendname,date,state,msgrecord,type) "
                                                     "values('%s', '%s', '%s', '%s', '%s', '%s');", mysendname, sendname, time_str, history, filename, "file"); // 这边保存的记录是文件名字
    rc = sqlite3_exec(db, sql_msg_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stdout, "insert file record success!\n");
    }

    return ;
}

// 选择好友，打开聊天框
static void do_choose_friend(MSG* msg, int client_socket, sqlite3* db, char* mysendname)    // mysendname是发送人的名字
{
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    MSG cur;
    int row;
    char temp[BUFFER_SIZE];
    int offset = 0;
    char* myname = msg->name;      // 自己名字
    char* friendname = msg->data;  // 好友名字
    char sql_friend_box_buffer[SQL_BUFFER_SIZE];
    char sql_state_buffer[BUFFER_SIZE] = {0};
    char friend_state[BUFFER_SIZE] = {0};

    strcpy(mysendname, myname);
    memset(&cur, 0, sizeof(cur));

    // 首先找好友列表，有没有该好友
    snprintf(sql_friend_box_buffer, sizeof(sql_friend_box_buffer), "select * from friendlist where myname = '%s' and friendname = '%s';", myname, friendname);
    rc = sqlite3_get_table(db, sql_friend_box_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    if (nRows == 1)
    {
        strcpy(cur.name, "ok");

        // 找到该好友后，查看好友是否在线，顺便告知当前用户，因为打开聊天窗口的时候可以看到是否在线
        snprintf(sql_state_buffer, sizeof(sql_state_buffer), "select * from usr where name = '%s';", friendname);

        rc = sqlite3_get_table(db, sql_state_buffer, &results, &nRows, &nCols, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        // printf("state:[%s]\n", results[nCols + 3]);
        if (strcmp(results[nCols + 3], "1") == 0)
        {
            strcpy(friend_state, "online");
        }
        else if (strcmp(results[nCols + 3], "0") == 0)
        {
            strcpy(friend_state, "offline");
        }
        snprintf(cur.data, sizeof(cur.data), "The friend chat box has been opened and this friend state:[%s]", friend_state);
    }
    else
    {
        strcpy(cur.name, "no");
        strcpy(cur.data, "You don't have the friend, please enter the correct friend name");
    }
    sqlite3_free_table(results);
    
    cur.type = htonl(C);
    if (send(client_socket, &cur, sizeof(cur), 0) < 0)
    {
        perror("server send() person information");
        exit(EXIT_FAILURE);
    }

    return ;
}

// 当用户退出的时候在线状态置为0
static void set_offline(char* msg)
{
    char myname[BUFFER_SIZE] = {0};
    char sql_set_state_buffer[BUFFER_SIZE] = {0};
    char sql_search_state_buffer[BUFFER_SIZE] = {0};
    int rc;
    char* zErrMsg = NULL;
    char state[BUFFER_SIZE] = {0};
    char **results;
    int nRows, nCols;

    strcpy(state, "0");
    strcpy(myname, msg);

    snprintf(sql_set_state_buffer, sizeof(sql_set_state_buffer), "update usr set state = '%s' where name = '%s';", state, myname);
        
    // sqlite3_exec(insert)
    rc = sqlite3_exec(db, sql_set_state_buffer, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s and change quit client state error\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        // 查询一下表是不是更新了在线状态（这边可能有bug）
        snprintf(sql_search_state_buffer, sizeof(sql_search_state_buffer), "select * from usr where name = '%s';", myname);
        rc = sqlite3_get_table(db, sql_search_state_buffer, &results, &nRows, &nCols, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        if (nRows == 1)
        {
            // fprintf(stdout, "change quit client state success, current state:[%s]\n", results[nCols + 3]);
        }
        sqlite3_free_table(results);
    }
}

// 查看与好友的聊天记录（查找record的表）【需要返回我发给对方的，也要返回对方发给我的】
/* 
例如，筛选出来的表如下：
    myname | friendname | date      | state   | msgrecord
    Bob    | Cindy      | 24.07.11  | history | "good!"
    Bob    | Cindy      | 24.07.12  | history | "nice!" 
此时 nRows = 3, nCols = 5 
*/
static void do_query_chat_record(MSG* msg, int client_socket, sqlite3* db)
{
    char myname[BUFFER_SIZE] = {0};
    char friendname[BUFFER_SIZE] = {0};
    char sql_chat_record_buffer[BUFFER_SIZE] = {0};
    int rc;
    char* zErrMsg = NULL;
    char **results;
    int nRows, nCols;
    int row = 0;
    MSG sendMsg;

    strcpy(myname, msg->name);
    strcpy(friendname, msg->data);

    snprintf(sql_chat_record_buffer, sizeof(sql_chat_record_buffer), "select * from record where (myname = '%s' and friendname = '%s') or (myname = '%s' and friendname = '%s');", myname, friendname, friendname, myname);
    rc = sqlite3_get_table(db, sql_chat_record_buffer, &results, &nRows, &nCols, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    if (nRows == 0)
    {
        sendMsg.type = htonl(E);
        strcpy(sendMsg.name, "no");
        strcpy(sendMsg.data, "You don't have a chat history with the user");
        if (send(client_socket, &sendMsg, sizeof(sendMsg), 0) < 0)
        {
            perror("send failed in login notsend");
            exit(EXIT_FAILURE);
        }
    }
    if (nRows != 0)
    {
        // 需要循环获取发送消息给当前用户，相当于返回查询记录
        for (row = 1; row <= nRows ; row++)
        {
            sendMsg.type = htonl(E);
            snprintf(sendMsg.name, sizeof(sql_chat_record_buffer), "%s-->%s", results[row * nCols], results[row * nCols + 1]);
            snprintf(sendMsg.data, sizeof(sql_chat_record_buffer), "%s : %s", results[row * nCols + 2], results[row * nCols + 4]);
            if (send(client_socket, &sendMsg, sizeof(sendMsg), 0) < 0)
            {
                perror("send failed in login notsend");
                exit(EXIT_FAILURE);
            }
        }
    }

    return ;
}

// 处理每一个连接上来的客户端（客户端线程）
static void *handle_client(void *arg)
{
    MSG readMsg;
    int valread;
    int client_socket = *(int *)arg;
    char client_ip[BUFFER_SIZE] = {0};
    char client_port[BUFFER_SIZE] = {0};
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char mysendname[BUFFER_SIZE];
    static char myname[BUFFER_SIZE]; // 静态变量，每次进来就不会初始化，可以保存上一次的名字

    // printf("client_socket = %d\n", client_socket);  // 调试语句
    getpeername(client_socket, (struct sockaddr *)&address, &addrlen); // 根据客户端的fd找到对应ip和端口
    strcpy(client_ip, inet_ntoa(address.sin_addr));
    snprintf(client_port, sizeof(client_port), "%d", ntohs(address.sin_port));

    // printf("New client connected, ip: %s, port: %s\n", client_ip, client_port);  // 调试语句

    while (1)
    {
        memset(&readMsg, 0, sizeof(readMsg));
        valread = read(client_socket, &readMsg, sizeof(readMsg));
        readMsg.type = ntohl(readMsg.type);  // Because it is 4 bytes, the network byte order needs to be changed
        if (valread == -1)
        {
            perror("read failed");
        }
        else if (valread > 0)
        {
            // printf("Client message num:%d, text:(%d, %s, %s)\n", valread, readMsg.type, readMsg.name, readMsg.data);
        }
        else if (valread == 0)   // Disconnecting means reading data as 0
        {
            // printf("Client disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            // 检测到客户退出，则此时服务器需要将该客户在usr表中的状态置为0，离线状态
            printf("quit name:%s\n", myname);   // 打印出退出的客户名字
            mark--;
            set_offline(myname); // 需要拿到当前客户的名字，去到usr表中找到对应行，将state置为0（offline）
            close(client_socket);
            break;
        }

        switch (readMsg.type)
        {
            case R: // 注册
                do_register(&readMsg, client_socket, db);
                break;
            case L: // 登录
                do_login(&readMsg, client_socket, db, client_ip, client_port);
                strcpy(myname, readMsg.name); // 登陆后保存用户名，方便后续退出时候可以操作
                break;
            case A: // 添加好友
                do_add_friend(&readMsg, client_socket, db);
                break;
            case Q: // 查询好友
                do_query_friendlist(&readMsg, client_socket, db);
                break;
            case D: // 删除好友
                do_delete_friend(&readMsg, client_socket, db);
                break;
            case C: // 选择好友聊天框（打开好友聊天框就可以看到是否在线）
                do_choose_friend(&readMsg, client_socket, db, mysendname);
                break;
            case S: // 给好友发信息（中转消息）
                do_msg_transit(&readMsg, client_socket, db, mysendname); // 发送人的姓名mysendname，好友姓名在readMsg中，发过去的消息也在readMsg中
                break;
            case F: // 给好友发文件（中转消息）
                do_file_transit(&readMsg, client_socket, db, mysendname, valread); // 发送人的姓名mysendname，好友姓名在readMsg中，发过去的消息也在readMsg中，valread是当前读取到的字节数
                break;
            case E: // 查看聊天记录
                do_query_chat_record(&readMsg, client_socket, db);
                break;
            case T: // 删除聊天记录
                do_delete_chat_record(&readMsg, client_socket, db);
                break;
            case O: // 注销账户（注销的同时一并将usr表中的name，好友记录表friendlist里面的包含该名字的，record记录表中myname包含该名字的一并全部删除）
                do_logout(&readMsg, client_socket, db);
                break;
            default:
                printf("Invalid data msg.\n");
                break;
        }
    }

    return NULL;
}

// socket初始化，创建监听套接字
static int socket_init()
{
    struct sockaddr_in address;
    int server_fd;
    int opt = 1;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) // 防止timewait和高并发复用端口
    {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	// address.sin_port = htons(atoi(argv[1]));      // 转端口号
    address.sin_port = htons(atoi(SERVER_PORT));
    address.sin_addr.s_addr = INADDR_ANY;            // 0.0.0.0
	// inet_aton("0.0.0.0", &address.sin_addr);      // 转ip（可以指定ip）
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) 
	{
		perror("bind failed");
        close(server_fd);
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, MAX_CLIENTS) < 0) 
	{
		perror("listen failed");
        close(server_fd);
		exit(EXIT_FAILURE);
	}

    return server_fd;
}

// 轻量级数据库sqlite初始化，以及创建三张存储数据的表
static int sqlite_init()
{
    char* zErrMsg = NULL;
    int rc;

    // sqlite3_open()
    if ((rc = sqlite3_open(DATA_INFO, &db)) == SQLITE_OK)
    {
        printf("open %s success\n", DATA_INFO);
    }
    else
    {
        printf("error:%s, %d\n", sqlite3_errmsg(db), rc);
        if (rc == 14)
        {
            printf("can not open.\n");
        }
        return -1;
    }

    // user table
    const char* sql_create_usr_table =   "create table usr("
                                         "name      text    primary key  not NULL,"
                                         "ip        text    not NULL,"
                                         "port      text    not NULL,"
                                         "state     text    not NULL,"
                                         "password  text    not NULL);";
    rc = sqlite3_exec(db, sql_create_usr_table, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -2;
    }
    else
    {
        fprintf(stdout, "Usr table created successfully!\n");
    }

    // friendlist table
    const char* sql_create_friendlist_table =   "create table friendlist("
                                                "myname         text    not NULL,"
                                                "friendname     text    not NULL);";
    rc = sqlite3_exec(db, sql_create_friendlist_table, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -3;
    }
    else
    {
        fprintf(stdout, "friendlist table created successfully!\n");
    }

    /* 一条信息就是一个记录，一直往下存，名字，日期，聊天记录
    * myname | friendname | date      | state   | msgrecord  | type
    * Alice  | Cindy      | 24.07.09  | history | "hello"    | msg
    * Alice  | Bob        | 24.07.10  | notsend | "nihao"    | msg
    * Bob    | Cindy      | 24.07.11  | notsend | "word.txt" | file
    */
    // record table
    const char* sql_create_record_table =   "create table record("
                                            "myname      text    not NULL,"
                                            "friendname  text    not NULL,"
                                            "date        text    not NULL,"
                                            "state       text    not NULL,"
                                            "msgrecord   text    not NULL,"
                                            "type        text    not NULL);";
    rc = sqlite3_exec(db, sql_create_record_table, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -4;
    }
    else
    {
        fprintf(stdout, "record table created successfully!\n");
    }

    return 0;
}

int main(int argc, char *argv[])
{
	int server_fd, new_socket;
    int i = 0;
    int ret = -1;
    struct sockaddr_in address;
    int addrlen = sizeof(struct sockaddr_in);
    int num_fds = 0;
    struct pollfd fds[1];
    pthread_t thread_ids[MAX_CLIENTS];
    pthread_t tid;

	// if (argc != 2)
    // {
	// 	printf("Usage: EXE + PORT; example:[./server 8888]\n");
	// 	exit(EXIT_FAILURE);
	// }

    // sqlite_init
    if (ret = sqlite_init())
    {
        printf("sqlite_init failed, errno = %d\n", ret);
        exit(EXIT_FAILURE);
    }

    // socket_init
    if ((server_fd = socket_init()) < 0)
    {
        printf("socket_init failed\n");
        exit(EXIT_FAILURE);
    }

    // 初始化
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        client_sockets[i] = 0;
        thread_ids[i] = 0;
    }

    // poll机制，减小cpu消耗
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    // accept + pthread_create
	while (1)
    {
        if ((num_fds = poll(fds, 1, -1)) == -1)
        {
            close(server_fd);
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        if (mark == 0)  // 服务器给任意客户发送消息的线程，只创建一次
        {
            pthread_create(&tid, NULL, send_client, NULL);
        }

        if (fds[0].revents & POLLIN)
        {
            // 接受客户端的连接
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == -1)
            {
                perror("accept failed");
		        exit(EXIT_FAILURE);
            }
            
            // 每个客户端都创建一个线程与之交互，且保存客户端的套接字在数组中
            client_sockets[mark] = new_socket;
            if (pthread_create(&thread_ids[mark], NULL, handle_client, (void *)&client_sockets[mark]) != 0)
            {
                perror("pthread_create failed\n");
                close(new_socket);
            }

            mark++; // 客户数量
            printf("client num:%d\n", mark);
        }
	}

    close(server_fd);

	return 0;
}