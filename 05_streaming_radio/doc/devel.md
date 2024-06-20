# IPv4 流媒体广播项目

Makefile和makefile都可以使用，优先调用小写的makefile，程序员写Makefile，把主动权留给用户去写小写的也行。

想去修改Makefile，就复制写一份makefile；到时候执行make就是执行小写的makefile。



## 1、通信协议

在C语言中，结构体定义中的数组字段`uint8_t data[1];`有一个字节的数组，常常用于灵活数组成员（flexible array member）技巧的变体，尽管它不是标准的灵活数组成员。这种定义允许在分配结构体时，动态地分配更大的数组。

**解释**

让我们详细解释这个结构体定义：

```c
struct msg_channel_st {
    uint8_t chnid;          // 表示频道ID，范围在 [MINCHNID, MAXCHNID]
    uint8_t data[1];        // 实际上是一个占位符，用于灵活数组
} __attribute__((packed));
```

- **`uint8_t chnid`**: 这是一个 8 位无符号整数，用于存储频道ID。
- **`uint8_t data[1]`**: 这是一个 8 位无符号整数数组，占用了 1 个字节，但它实际上是作为占位符使用，用于表示数据的起始地址。

**`__attribute__((packed))`**

这个编译器特性指示编译器不要在结构体成员之间添加任何填充字节（padding），从而紧凑地排列结构体中的成员。这在需要严格控制内存布局时非常有用，例如在嵌入式系统或网络协议实现中。

**灵活数组成员的用法**

虽然 `uint8_t data[1];` 不是标准的灵活数组成员，但它实现了类似的目的。标准的灵活数组成员定义为 `uint8_t data[];`，没有指定大小。为了与旧版代码或特定编译器兼容，有时使用`uint8_t data[1];`来实现相同的效果。

**动态分配结构体**

通过这种定义，你可以在分配结构体时动态分配更多的内存来存储数据。以下是如何使用这种技巧的示例：

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MINCHNID 1
#define MAXCHNID 255

struct msg_channel_st {
    uint8_t chnid;          // must between [MINCHNID, MAXCHNID] 表示id号
    uint8_t data[1];
} __attribute__((packed));

int main() {
    // 假设我们需要10字节的数据
    size_t data_size = 10;
    struct msg_channel_st *msg;
    
    // 分配内存，包含结构体的固定部分和额外的数据部分
    msg = (struct msg_channel_st *)malloc(sizeof(struct msg_channel_st) + data_size - 1);
    if (msg == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // 初始化结构体成员
    msg->chnid = 1;  // 设置频道ID
    memcpy(msg->data, "1234567890", data_size);  // 复制数据到data字段

    // 输出结果以验证
    printf("Channel ID: %d\n", msg->chnid);
    printf("Data: %.*s\n", (int)data_size, msg->data);

    // 释放内存
    free(msg);

    return 0;
}
```

**解释示例代码**

1. **定义结构体**：结构体`msg_channel_st`具有一个 8 位的频道ID和一个占位的 8 位数据数组。
2. **分配内存**：使用 `malloc` 分配足够的内存来存储结构体和额外的数据部分。注意，分配大小为 `sizeof(struct msg_channel_st) + data_size - 1`，这里减去1是因为`data`数组已经占用了一个字节。
3. **初始化成员**：设置频道ID并将数据复制到`data`数组中。
4. **输出验证**：打印频道ID和数据以验证程序是否正确。
5. **释放内存**：使用 `free` 释放分配的内存。

通过这种方式，你可以灵活地在运行时确定 `data` 数组的大小，而不是在编译时。



灵活数组成员（Flexible Array Member，FAM）是C语言的一种特性，它允许在结构体中定义一个大小可变的数组，以便在结构体末尾灵活地存储不定长的数据。这种特性使得结构体可以适应不同大小的数据，而无需在编译时确定数组的具体大小。

**灵活数组成员的定义**

灵活数组成员是在结构体的最后一个成员定义为一个未指定大小的数组。例如：

```c
struct msg_channel_st {
    uint8_t chnid;  // 表示ID号，范围在 [MINCHNID, MAXCHNID]
    uint8_t data[]; // 灵活数组成员
};
```

注意，灵活数组成员 `data` 没有指定大小，这使得它可以在结构体末尾根据需要分配不同大小的内存。

**使用灵活数组成员**

使用灵活数组成员时，通常需要动态分配内存，以便为数组预留足够的空间。这可以通过 `malloc` 或其他内存分配函数来实现。以下是一个示例代码，展示了如何使用灵活数组成员：

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MINCHNID 1
#define MAXCHNID 255

struct msg_channel_st {
    uint8_t chnid;  // 表示ID号，范围在 [MINCHNID, MAXCHNID]
    uint8_t data[]; // 灵活数组成员
};

int main() {
    // 假设我们需要10字节的数据
    size_t data_size = 10;
    struct msg_channel_st *msg;

    // 分配内存，包含结构体的固定部分和额外的数据部分
    msg = (struct msg_channel_st *)malloc(sizeof(struct msg_channel_st) + data_size);
    if (msg == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // 初始化结构体成员
    msg->chnid = 1;  // 设置频道ID
    memcpy(msg->data, "1234567890", data_size);  // 复制数据到data字段

    // 输出结果以验证
    printf("Channel ID: %d\n", msg->chnid);
    printf("Data: %.*s\n", (int)data_size, msg->data);

    // 释放内存
    free(msg);

    return 0;
}
```

**解释示例代码**

1. **定义结构体**：结构体 `msg_channel_st` 有一个 8 位的频道ID 和一个灵活数组成员 `data`。
2. **分配内存**：使用 `malloc` 分配足够的内存来存储结构体的固定部分和额外的数据部分。分配大小为 `sizeof(struct msg_channel_st) + data_size`。
3. **初始化成员**：设置频道ID 并将数据复制到 `data` 数组中。
4. **输出验证**：打印频道ID 和数据以验证程序是否正确。
5. **释放内存**：使用 `free` 释放分配的内存。

**为什么使用灵活数组成员？**

灵活数组成员有以下几个优点：

1. **节省内存**：通过仅分配实际需要的内存，避免了预定义大小的数组可能带来的浪费。
2. **提高效率**：能够灵活地处理不定长的数据，提高程序的灵活性和效率。
3. **简化代码**：减少了对多余数组大小的管理，使代码更加简洁和易于维护。

**需要注意的事项**

- **灵活数组成员必须是结构体中的最后一个成员**。
- **在动态分配内存时，要确保分配足够的空间以容纳结构体和数组数据**。
- **灵活数组成员只能用于动态内存分配，不能用于自动（栈）分配的结构体实例**。

灵活数组成员是C语言中的一个强大特性，允许结构体更高效地处理可变长度的数据，同时保持内存使用的灵活性和效率。



thr_channel 每个线程往外发数据，就是发每一个节目的视频流，流量控制来控制发多少；

thr_list 节目单也用一个线程往外发，持续一段时间的发，每秒发一次节目单；

s端负责将每个频道的数据以及节目单每秒往外发送，客户端接受，拿到节目单，打印出来，选择某个频道开始听。

选择该频道后，服务器就给你发这个频道数据即可。



## 2、客户端搭建

跨网络的数据传输不能直接使用指针的主要原因是指针是针对特定内存地址的引用，而不是真正的数据。当你在本地计算机上使用指针时，它指向的是本地内存中的数据。但是，如果你尝试将指针直接传输到另一个计算机上，该指针实际上只是一个地址，这个地址在远程计算机上没有对应的数据。

在进行网络数据传输时，你需要将数据序列化为字节流，并通过网络发送。使用指针来传输数据会导致以下问题：

1. **地址空间不同：** 不同计算机的内存地址空间是不同的，所以一个计算机上的指针在另一个计算机上没有意义。

2. **无法访问远程内存：** 不能直接访问另一台计算机上的内存，因此无法通过指针访问远程计算机上的数据。

因此，在进行跨网络的数据传输时，需要将数据转换为适合网络传输的格式，例如使用协议缓冲区（Protocol Buffers）、JSON（JavaScript Object Notation）等进行序列化，然后通过网络发送这些序列化的数据。在接收端，需要解析接收到的字节流并将其转换回原始数据格式。



`getopt_long()` 函数的参数包括：

1. **argc**：命令行参数的数量，通常是 `main()` 函数的参数 `argc`。

2. **argv**：命令行参数的数组，通常是 `main()` 函数的参数 `argv`。

3. **optstring**：包含短选项的字符串，表示程序可以接受的短选项。每个短选项字符后面可以跟一个冒号 `:` 表示该选项需要一个参数，如果没有参数则可以省略。如果该字符串以冒号开头，则表示禁用 `getopt_long()` 的错误提示，而由程序自己处理错误。

4. **longopts**：一个指向 `struct option` 结构体数组的指针，用于定义长选项。每个 `struct option` 结构体包含长选项的名称、是否需要参数以及指向存储长选项的索引的变量的指针。

5. **longindex**：一个指向存储长选项索引的变量的指针，用于存储 `getopt_long()` 返回的长选项的索引。

下面是 `struct option` 结构体的定义：

```c
struct option {
    const char *name; // 长选项的名称
    int         has_arg; // 是否需要参数：no_argument（0）、required_argument（1）、optional_argument（2）
    int        *flag; // 如果不为NULL，会将val的值赋给flag指向的int型变量，并且返回0，否则返回val
    int         val; // 用于标识长选项的整数值
};
```

`getopt_long()` 函数返回值为解析到的选项字符。如果所有选项解析完毕，则返回 -1。



man 7 ip


`ip_mreqn` 这个词的每个部分可以拆分为以下含义：

- **ip**：这是 Internet Protocol 的缩写，代表互联网协议。在网络编程中，通常指 IPv4 或 IPv6 协议。

- **mreq**：这代表 Multicast Request（多播请求），表明这个结构体是用于设置或处理多播相关的信息。

- **n**：可能代表 "new"，表示这个结构体是一个用于新版操作系统或者新版协议的结构体。

所以整体来看，`ip_mreqn` 可能代表了一个用于处理新版多播请求的 IPv4 相关的结构体。



`IP_MULTICAST_LOOP` 是一个用于控制多播数据包是否发送到本地接收者的套接字选项。当启用这个选项时，多播数据包将被发送到同一个主机上的其他套接字，即使这些套接字加入了相同的多播组。当禁用这个选项时，多播数据包将不会发送到同一个主机上的其他套接字。

这个选项的值是一个整数，可以为 0 或者 1。通常情况下，启用该选项将其设置为 1，禁用则设置为 0。

以下是一个简单的示例，演示了如何使用 `setsockopt` 函数来启用或禁用 `IP_MULTICAST_LOOP` 选项：

```c
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MULTICAST_ADDR "239.0.0.1"
#define PORT 12345

int main() {
    int sockfd;
    struct sockaddr_in addr;
    int loop = 1; // 启用多播循环

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置多播地址和端口
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 本地接收所有地址
    addr.sin_port = htons(PORT);

    // 将套接字绑定到端口
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 启用或禁用多播循环
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) == -1) {
        perror("setsockopt");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Multicast loop enabled\n");

    // 在这里可以发送或接收多播数据...

    // 关闭套接字
    close(sockfd);

    return 0;
}
```

在这个示例中，我们通过 `setsockopt` 函数设置了 `IP_MULTICAST_LOOP` 选项，启用了多播循环功能。这意味着多播数据包将被发送到同一主机上的其他套接字。



`IP_MULTICAST_LOOP` 是一个用于控制多播数据包是否发送到本地接收者的套接字选项。

当启用 `IP_MULTICAST_LOOP` 选项时，多播数据包将被发送到同一主机上的其他套接字，即使这些套接字加入了相同的多播组。这意味着本地主机上的多播发送者也能接收到发送的数据包。这在某些场景下是有用的，比如在实现点对多点通信时，可以通过启用多播循环来让发送者也能够接收到发送的数据，从而实现简化的通信逻辑。

而当禁用 `IP_MULTICAST_LOOP` 选项时，多播数据包将不会发送到本地接收者，即使它们加入了相同的多播组。这在某些场景下也是有用的，比如在某些应用中，本地发送者不需要接收发送的数据包，可以通过禁用多播循环来避免发送者接收到自己发送的数据包，从而减少网络流量和资源消耗。

总之，`IP_MULTICAST_LOOP` 选项用于控制多播数据包在本地发送者上的处理方式，从而满足不同的通信需求。





`dup2` 是一个系统调用，用于复制文件描述符。它允许将一个文件描述符复制到另一个指定的文件描述符上。如果目标文件描述符已经打开，则会先关闭目标文件描述符，然后再进行复制。

函数签名如下：

```c
#include <unistd.h>

int dup2(int oldfd, int newfd);
```

参数说明：

- `oldfd`：要复制的源文件描述符。
- `newfd`：要将源文件描述符复制到的目标文件描述符。

函数返回值：

- 如果复制成功，则返回新的文件描述符。
- 如果复制失败，则返回 -1，并设置 `errno` 来指示错误的原因。

`dup2` 主要用于重定向文件描述符，例如将标准输出重定向到文件中，或者将一个套接字连接到标准输入等。`dup2` 函数在文件描述符的复制过程中保持了文件描述符的原子性，因此在多线程环境中也是安全的。

以下是一个简单的示例，演示了如何使用 `dup2` 函数将标准输出重定向到一个文件中：

```c
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd;

    // 打开一个文件，用于重定向标准输出
    fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 将标准输出重定向到打开的文件描述符上
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return 1;
    }

    // 现在标准输出将写入到 output.txt 文件中
    printf("This will be written to the file.\n");

    // 关闭文件描述符
    close(fd);

    return 0;
}
```

在这个示例中，通过 `dup2` 函数将打开的文件描述符复制到标准输出的文件描述符上，从而实现了标准输出的重定向。现在，所有写入到标准输出的内容都会被重定向到指定的文件中。



mpg ***.mp3 - （加个-减号代表从标准输入获取信息数据流）


```bash
cat baby\ i\'m\ sorry.mp3  | mpg123 - # cat一个mp3文件，通过管道以标准输入的方式使mpg123播放（将cat命令的标准输出作为了另外一个命令的标准输入）
```

在 Linux 命令行中，`|` 符号表示管道（Pipe）操作符，用于将一个命令的输出传递给另一个命令的输入。通过管道操作符，可以将多个命令组合起来，构建更复杂的命令链，从而实现更强大的功能。

例如，假设有两个命令 `command1` 和 `command2`，可以通过 `|` 符号将它们连接起来，如下所示：

```
command1 | command2
```

这将把 `command1` 的输出作为 `command2` 的输入。也就是说，`command1` 的输出结果将会直接传递给 `command2`，作为 `command2` 的输入。这样，`command2` 将会处理 `command1` 的输出，并产生最终的结果。

管道操作符的使用使得 Linux 命令行非常强大和灵活，因为它允许将各种命令组合在一起，以实现更复杂的数据处理和操作。



```c
dup2(pipefd[0], 0);
if (pipefd[0] > 0)
    close(pipefd[0]);
```

在调用 `dup2(pipefd[0], 0)` 之后，文件描述符 `0` 已经指向了管道的读端，即标准输入已经被重定向到了管道。这时，如果程序中不再需要直接操作 `pipefd[0]`，就可以关闭它了。

关闭 `pipefd[0]` 有几个好处：

1. **资源释放**：关闭不再使用的文件描述符可以释放资源，防止资源泄漏。

2. **避免误操作**：关闭 `pipefd[0]` 可以避免程序中误操作，因为 `pipefd[0]` 和标准输入现在都指向同一个管道的读端，如果程序在后续的逻辑中意外地操作了 `pipefd[0]`，可能会导致预期之外的结果。

3. **清理环境**：关闭文件描述符可以清理程序运行环境，使代码更加简洁和清晰。

因此，通常在将标准输入重定向到管道后，会关闭对应的管道读端文件描述符。



直接sh可以调用sh终端：


```c
struct msg_listentry_st {
    uint8_t chnid;          // 节目频道号                             1 
    uint16_t len;           // len表示此时该结构体是多大，占多少字节，这样才能知道后续哪里是id那里是内容
    uint8_t desc[];         // 节目描述可长可短，如下节目单的样式       music:baby I'm sorry
}__attribute__((packed));

struct msg_list_st {        // 0号频道发节目单
    uint8_t chnid;          // must be LISTCHNID     0号频道
    struct msg_listentry_st entry[];    // 节目单的样式：1 music:baby I'm sorry / 2 sport:football
}__attribute__((packed));
```

struct msg_list_st 结构体示例；后面每个都是变长结构体msg_listentry_st，每个这个结构体内又是左边的形式，频道id加变长内容。


int  *pos; pos++；是移动了4个字节吗

是的，如果 `pos` 是一个指向 `int` 类型的指针，那么 `pos++` 操作会使 `pos` 指向下一个 `int` 类型的位置，即移动了 `4` 个字节（在大多数平台上，`int` 类型通常占据 `4` 个字节的内存空间）。

这是因为在指针运算中，指针的类型决定了移动的距离。指针加上 `1` 时，实际上增加了 `sizeof(指针类型)` 个字节的偏移量。



`ssize_t` 和 `size_t` 是两个 C 语言中的数据类型。

- `size_t` 是无符号整数类型，用来表示对象的大小（以字节为单位），例如数组的长度、内存分配的大小等。它的大小在不同的编译器和平台上可能会有所不同，但通常被定义为足以容纳系统中最大可能对象的类型。在标准库中，`size_t` 通常是 `stddef.h` 或 `stdlib.h` 头文件中定义的 `typedef`。

- `ssize_t` 是带符号整数类型，通常用于表示函数的返回值或者错误码。它的大小也会因平台而异，但通常足够大以容纳函数的返回值或者指示错误状态。`ssize_t` 是 POSIX 标准中定义的类型，通常在 `unistd.h` 或 `sys/types.h` 中。

在很多情况下，`size_t` 用于表示内存分配和数组索引，而 `ssize_t` 用于表示文件大小、读取或写入的字节数等。



## 3、服务器端搭建

thr_list 创建线程每秒发送一次节目单。

thr_channel 创建线程发送频道内容数据。

medialib 是媒体库。 



`CFLAGS` 和 `LDFLAGS` 是两个常见的 Makefile 变量，用于存储编译器选项和链接器选项。

- `CFLAGS` 变量用于存储编译器选项。在这个 Makefile 中，`-I ../include` 表示添加一个头文件搜索路径，`-pthread` 表示启用线程支持。所以 `CFLAGS+=-I ../include -pthread` 的作用是将这些选项添加到编译器选项中。

- `LDFLAGS` 变量用于存储链接器选项。在这个 Makefile 中，`-pthread` 表示链接时需要包含线程库。所以 `LDFLAGS+=-pthread` 的作用是将这些选项添加到链接器选项中。

综合起来，这两个语句用于在编译和链接时分别添加线程支持选项，以及指定头文件的搜索路径。



像这个init进程就是守护进程，首先他父进程是1号init进程，pid进程号，pgid组id，sid会话组id都是一样的，代表他自己就是新会话且是守护进程，tty为问号说明脱离控制终端。



像ftp服务都是守护进程，一开机就一直在后台跑，任何时候当前都能使用。



当调用 `pause()` 函数时，进程会挂起并等待信号到来。在等待信号期间，进程会进入睡眠状态，即不会被调度到CPU上执行。这意味着进程会暂时放弃CPU的使用权，直到收到一个信号，并且被信号处理函数唤醒。

在这种情况下，调度器会将CPU分配给其他准备好的进程，直到挂起的进程被唤醒。因此，`pause()` 函数在等待信号时是非活动状态，不会占用CPU资源。



`getopt` 是一个用于解析命令行参数的C语言标准库函数。它允许你轻松地处理命令行参数，并且通常与循环结合使用，以获取命令行中的每个参数及其值。以下是使用 `getopt` 函数的基本用法示例：

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    char *output_file = NULL;

    // 循环处理命令行参数
    while ((opt = getopt(argc, argv, "i:o:")) != -1) {
        switch (opt) {
            case 'i':
                // 获取选项参数的值，保存到 input_file 中
                input_file = optarg;
                break;
            case 'o':
                // 获取选项参数的值，保存到 output_file 中
                output_file = optarg;
                break;
            case '?':
                // 处理未识别的选项
                fprintf(stderr, "Unknown option: %c\n", optopt);
                return EXIT_FAILURE;
        }
    }

    // 处理完命令行参数后，optind 是下一个待处理的参数的索引
    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);

    // 处理其他非选项参数
    for (int i = optind; i < argc; i++) {
        printf("Non-option argument: %s\n", argv[i]);
    }

    return EXIT_SUCCESS;
}
```

**解释**

- `getopt` 函数会依次读取命令行参数，并返回每个选项的简称（如果有的话）。
- `getopt` 的第三个参数是一个字符串，表示允许的选项。每个字符代表一个选项，后面可以加一个冒号 `:`，表示该选项需要一个值。
- `getopt` 在处理完所有选项后返回 -1。
- `optarg` 是一个全局变量，它包含了当前选项的参数值。
- `optind` 是 `getopt` 处理完所有选项后的索引，表示下一个待处理的参数。

**示例用法**

假设你的程序名为 `example`，以下是一些示例命令行用法及其对应的输出：

```sh
./example -i input.txt -o output.txt arg1 arg2
```

输出：

```
Input file: input.txt
Output file: output.txt
Non-option argument: arg1
Non-option argument: arg2
```

```sh
./example -i -o output.txt arg1 arg2
```

输出：

```
Unknown option: i
```

```sh
./example -i input.txt -o arg1 arg2
```

输出：

```
Input file: input.txt
Output file: arg1
Non-option argument: arg2
```

**注意事项**

- 使用 `getopt` 需要包含头文件 `<unistd.h>`。
- 命令行参数中的选项可以是单个字符，也可以是长选项（例如 `--input-file`），但是 `getopt` 只处理简称选项。
- 如果一个选项需要一个值，则在选项后面加一个冒号 `:`，例如 `"i:o:"`。
- 当命令行参数中有未识别的选项时，`getopt` 返回 `?`，并将未识别的选项存储在全局变量 `optopt` 中。



`setsid` 函数用于创建一个新的会话（session）。在 POSIX 操作系统（如 Unix 或类 Unix 系统）中，可以使用 `setsid` 函数来启动一个新的会话，使得调用进程成为该会话的领头进程（session leader）。`setsid` 函数创建了一个新的会话，其中包含了一个新的进程组（process group），该进程组中只包含了一个进程：调用 `setsid` 的进程。

这个函数通常由守护进程（daemon）使用，以确保它们不受控制终端（controlling terminal）的影响。因为一个会话中只有一个控制终端，而调用 `setsid` 后，新的会话将不再有控制终端。

在 C 语言中，`setsid` 函数的原型通常在 `<unistd.h>` 头文件中声明，其原型如下：

```c
pid_t setsid(void);
```

该函数调用成功时返回新会话的会话 ID（session ID），如果失败则返回 -1，并设置 `errno` 来指示错误的原因。



具体来说，如果在函数内部调用 `fork()`，然后函数退出，那么父进程和子进程将会执行主函数中的代码，而不再受到函数的影响。这意味着，父子进程都可以继续执行主函数中的内容，包括分支和条件语句中的代码。



`umask(0)` 是一个系统调用，用于设置文件创建掩码（file mode creation mask）。在 Unix 和类 Unix 系统中，文件创建掩码决定了新文件的默认权限。`umask(0)` 的作用是清除当前进程的文件创建掩码，使得新创建的文件不受任何限制，即文件权限被设置为最大允许值。

具体来说，文件创建掩码是一个权限掩码，它在新文件的权限上起作用。当新文件被创建时，它的权限会从所使用的创建掩码中移除对应的位。通常，创建掩码会从当前进程继承或者在程序启动时设置。通过调用 `umask(0)`，可以清除当前进程的文件创建掩码，以便新文件拥有最大的权限。

请注意，`umask(0)` 只会影响当前进程，而不会影响其他进程或系统的文件创建掩码。



`openlog` 是一个函数，通常用于在 C 语言中打开系统日志（system log）。它是 POSIX 标准的一部分，可用于在程序中将消息发送到系统日志。

`openlog` 函数的原型通常在 `<syslog.h>` 头文件中声明，其原型如下：

```c
void openlog(const char *ident, int option, int facility);
```

- `ident` 参数是一个字符串，用于指定程序在系统日志中的标识符。这个标识符通常是程序的名称。
- `option` 参数指定了一些选项，用于配置日志记录的行为。可以使用 `LOG_CONS`、`LOG_NDELAY`、`LOG_NOWAIT`、`LOG_ODELAY`、`LOG_PERROR` 和 `LOG_PID` 等选项。这些选项的具体含义可以在相关文档中找到。
- `facility` 参数指定了一个系统设施（facility），用于指示消息的类型。比如 `LOG_USER`、`LOG_LOCAL0`、`LOG_LOCAL1` 等等。不同的设施对应不同的日志文件，比如 `LOG_USER` 对应 `/var/log/user.log`。

调用 `openlog` 函数后，程序就可以使用 `syslog` 函数来向系统日志发送消息。通常情况下，`openlog` 函数会在程序启动时调用，用于初始化系统日志。



`SIGTERM` 的全称是 "Signal Terminate"。

`SIGTERM` 是一个 POSIX 标准信号，用于请求进程终止。当操作系统或其他进程向目标进程发送 `SIGTERM` 信号时，目标进程通常应该以正常的方式退出。这个信号允许进程在收到终止请求时进行清理工作，保存数据等操作，然后优雅地退出。

通常情况下，`SIGTERM` 会被视为“温柔”的终止请求，因为它允许进程有机会进行清理操作。相比之下，`SIGKILL` 信号是一个强制终止信号，不允许目标进程做任何清理工作，直接终止进程。

在 UNIX 系统中，可以使用命令行工具 `kill` 来向进程发送信号，例如：

```
kill -TERM <PID>
```

这个命令会向进程 `<PID>` 发送 `SIGTERM` 信号，请求进程正常退出。



`SIGQUIT` 和 `SIGINT` 都是 POSIX 标准中定义的信号。

1. `SIGQUIT`，全称 "Signal Quit"，是一个终止信号。当用户在终端中按下 Ctrl+\（通常是 Ctrl+反斜杠）时，会发送 `SIGQUIT` 信号给当前运行的进程。与 `SIGTERM` 类似，但 `SIGQUIT` 的默认行为是终止进程并生成核心转储（core dump），这是一种调试工具，用于分析进程在终止时的内存状态。

2. `SIGINT`，全称 "Signal Interrupt"，是一个中断信号。当用户在终端中按下 Ctrl+C 时，会发送 `SIGINT` 信号给当前运行的进程。这是一种常见的中断信号，通常用于请求进程终止或中断当前的操作。

这两种信号都可以通过编程来捕获和处理，例如使用 `signal` 函数或者更复杂的信号处理机制，以便进程可以在收到信号时采取适当的行动，比如清理资源并安全退出。





多播组的IP地址可以被多个进程使用。这是多播通信的一个关键特性，允许多个进程在同一个主机上（甚至在不同的主机上）加入同一个多播组，并接收发送到该多播组的数据包。

具体来说，当多个进程在同一台主机上加入同一个多播组时，网络接口会被配置为接收发送到该多播组IP地址的数据包。这些数据包会被传递到所有加入该多播组并绑定到相同端口的进程。

**示例说明**

假设有两个进程在同一台主机上运行，并且它们都加入了多播组239.0.0.1，端口12345：

1. **进程A**：
   - 创建一个UDP套接字。
   - 绑定到本地端口12345。
   - 加入多播组239.0.0.1。

2. **进程B**：
   - 创建另一个UDP套接字。
   - 同样绑定到本地端口12345。
   - 加入多播组239.0.0.1。

当一个数据包被发送到多播组239.0.0.1的端口12345时，这两个进程都会收到该数据包。

**示例代码**

下面是一个简单的C语言示例，展示如何编写两个进程加入同一个多播组并接收数据：

**进程A和进程B的代码是相同的，只需分别启动两个实例即可：**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sock;
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    char buffer[256];
    
    // 创建UDP套接字
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    // 设置多播组地址和端口
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 任意本地地址
    addr.sin_port = htons(12345);  // 多播端口

    // 绑定到多播端口
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 设置多播组地址
    mreq.imr_multiaddr.s_addr = inet_addr("239.0.0.1");  // 多播组地址
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);  // 本地接口

    // 加入多播组
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    // 接收多播数据
    while (1) {
        int n = recv(sock, buffer, sizeof(buffer), 0);
        if (n < 0) {
            perror("recv");
            exit(1);
        }
        buffer[n] = '\0';
        printf("Received: %s\n", buffer);
    }

    // 关闭套接字
    close(sock);

    return 0;
}
```

**启动和测试**

1. 编译代码生成可执行文件，例如 `multicast_receiver`。

2. 启动两个实例：

   ```sh
   ./multicast_receiver &
   ./multicast_receiver &
   ```

3. 发送一个数据包到多播组（可以使用另一个简单的UDP发送程序）：

   ```c
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <arpa/inet.h>
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   
   int main() {
       int sock;
       struct sockaddr_in addr;
       char *message = "Hello Multicast Group!";
       
       // 创建UDP套接字
       if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
           perror("socket");
           exit(1);
       }
   
       // 设置目标地址和端口
       memset(&addr, 0, sizeof(addr));
       addr.sin_family = AF_INET;
       addr.sin_addr.s_addr = inet_addr("239.0.0.1");  // 多播组地址
       addr.sin_port = htons(12345);  // 多播端口
   
       // 发送数据包到多播组
       if (sendto(sock, message, strlen(message), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
           perror("sendto");
           exit(1);
       }
   
       // 关闭套接字
       close(sock);
   
       return 0;
   }
   ```

通过这种方式，两个进程都能接收到发送到多播组的数据包，从而实现多播通信的目的。这种特性在需要多进程协作的应用场景中非常有用。



多播组的ip被共用，发送端即服务器往多播组ip指定端口发，任何一个进程都可以加入该多播组接受消息。



## 4、流量控制

令牌桶流量控制；



"TBF" 在计算机网络中通常指的是 "Token Bucket Filter"，中文可以翻译为 "令牌桶过滤器"。它被称为流量控制的一种方式，因为它通过管理令牌桶中的令牌来控制数据包的发送速率。

具体来说，令牌桶过滤器工作原理如下：在发送数据包之前，需要从令牌桶中获取令牌。每个令牌对应于可以发送的一个数据包。如果令牌桶中没有足够的令牌，那么数据包就需要等待，直到有足够的令牌为止。这样一来，通过调整令牌生成的速率和令牌桶的大小，就可以有效地控制网络流量的发送速率，避免网络拥塞和过载。

因此，TBF被称为流量控制，是因为它能够有效地控制数据包的发送速率，确保网络的稳定和可靠性。



在多线程编程中，条件变量（condition variables）用于线程间的等待和通知机制，通常与互斥锁（mutex）结合使用。你提到的代码结构是经典的条件变量使用模式，用于等待某个条件满足并且防止虚假唤醒（spurious wakeup）。让我们逐步解释这段代码，并通过一个例子说明其用途。

**代码解释**

```c
pthread_mutex_lock(&mut);
while(token <= 0) 
    pthread_cond_wait(&cond, &mut);
```

1. **锁定互斥锁** (`pthread_mutex_lock(&mut);`):
   - 互斥锁 `mut` 用于保护共享资源 `token`。在访问和修改 `token` 之前，需要加锁以确保只有一个线程可以访问 `token`。

2. **条件检查和等待** (`while(token <= 0) pthread_cond_wait(&cond, &mut);`):
   - `while` 循环检查共享资源 `token` 是否满足某个条件（这里是 `token > 0`）。如果 `token <= 0`，则线程调用 `pthread_cond_wait` 进入等待状态。
   - `pthread_cond_wait` 做了两件事：它将线程放入等待队列，并原子性地解锁互斥锁 `mut`，以便其他线程可以修改 `token`。
   - 当条件变量 `cond` 被唤醒时，`pthread_cond_wait` 重新加锁 `mut` 并返回，线程继续执行检查条件。

**为什么使用 `while` 循环**

- **防止虚假唤醒**:（每次都会while循环来检查条件，if不会重复判断条件）
  - 条件变量可能会发生虚假唤醒，即线程被唤醒但条件仍未满足。如果不使用 `while` 循环，仅使用 `if` 检查，线程在虚假唤醒后将继续执行，这可能导致逻辑错误。
  - 使用 `while` 循环可以确保线程在被唤醒后再次检查条件，只有在条件满足时才会退出循环继续执行。

**举例说明**

以下是一个生产者-消费者问题的示例，其中生产者生产数据，消费者消费数据，使用条件变量和互斥锁来同步它们的操作：

```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE 10
int buffer[BUFFER_SIZE];
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;

void* producer(void* arg) {
    for (int i = 0; i < 20; ++i) {
        pthread_mutex_lock(&mutex);

        while (count == BUFFER_SIZE) // Buffer full, wait for consumer to consume
            pthread_cond_wait(&cond_not_full, &mutex);

        buffer[count++] = i; // Produce an item
        printf("Produced: %d, Buffer Count: %d\n", i, count);

        pthread_cond_signal(&cond_not_empty); // Signal consumer that buffer is not empty

        pthread_mutex_unlock(&mutex);

        sleep(1); // Simulate time taken to produce an item
    }
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < 20; ++i) {
        pthread_mutex_lock(&mutex);

        while (count == 0) // Buffer empty, wait for producer to produce
            pthread_cond_wait(&cond_not_empty, &mutex);

        int item = buffer[--count]; // Consume an item
        printf("Consumed: %d, Buffer Count: %d\n", item, count);

        pthread_cond_signal(&cond_not_full); // Signal producer that buffer is not full

        pthread_mutex_unlock(&mutex);

        sleep(2); // Simulate time taken to consume an item
    }
    return NULL;
}

int main() {
    pthread_t prod_thread, cons_thread;

    pthread_create(&prod_thread, NULL, producer, NULL);
    pthread_create(&cons_thread, NULL, consumer, NULL);

    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_not_empty);
    pthread_cond_destroy(&cond_not_full);

    return 0;
}
```

**解释**

- **生产者线程**:
  - 检查缓冲区是否已满 (`count == BUFFER_SIZE`)。
  - 如果满了，调用 `pthread_cond_wait` 等待消费者消耗数据并释放互斥锁。
  - 如果不满，生产一个数据并将其放入缓冲区，然后发送信号通知消费者缓冲区不为空 (`pthread_cond_signal(&cond_not_empty)`)，最后解锁互斥锁。

- **消费者线程**:
  - 检查缓冲区是否为空 (`count == 0`)。
  - 如果为空，调用 `pthread_cond_wait` 等待生产者生产数据并释放互斥锁。
  - 如果不为空，消费一个数据并从缓冲区中移除，然后发送信号通知生产者缓冲区不满 (`pthread_cond_signal(&cond_not_full)`)，最后解锁互斥锁。

通过这种方式，生产者和消费者线程能够有效地同步工作，确保数据的正确生产和消费，防止竞争条件和资源冲突。



## 7、媒体库实现

`glob` 是一个 POSIX 标准的函数，用于执行通配符匹配（globbing），即根据通配符模式查找文件或目录。它通常用于获取符合特定模式的文件路径列表。

**函数原型**

```c
#include <glob.h>

int glob(const char *pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *pglob);
```

- `pattern`: 匹配模式字符串，可以包含通配符 `*`（任意长度的任意字符）和 `?`（单个任意字符），以及 `[...]`（字符集）和 `[!...]`（反字符集）。
- `flags`: 控制匹配行为的标志，通常为 `0`。
- `errfunc`: 错误处理函数的指针，可以指定自定义的错误处理函数，通常为 `NULL` 使用默认错误处理。
- `pglob`: 指向 `glob_t` 结构体的指针，用于存储匹配结果。

**返回值**

- 返回值为 0 表示成功匹配。
- 返回值为 `GLOB_NOMATCH` 表示没有匹配的文件。
- 返回值为其他非零值表示出现错误，具体错误可以通过 `errno` 获取。

**使用示例**

以下是一个简单的示例，展示如何使用 `glob` 函数进行文件名的通配符匹配：

```c
#include <stdio.h>
#include <glob.h>

int main() {
    glob_t glob_result;
    int ret;

    // 匹配当前目录下所有 .txt 文件
    ret = glob("*.txt", 0, NULL, &glob_result);
    if (ret != 0) {
        if (ret == GLOB_NOMATCH) {
            printf("No matching files found.\n");
        } else {
            perror("glob");
            return 1;
        }
    }

    // 输出匹配到的文件路径
    printf("Matched %lu files:\n", (unsigned long)glob_result.gl_pathc);
    for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
        printf("%s\n", glob_result.gl_pathv[i]);
    }

    // 释放 glob_result 中动态分配的资源
    globfree(&glob_result);

    return 0;
}
```

**注意事项**

- 使用 `glob` 函数前要确保 `glob_t` 结构体已经初始化，可以通过 `glob_t glob_result = {0};` 或者 `glob_t glob_result = { .gl_pathc = 0 };` 进行初始化。
- 在使用完 `glob` 函数获取的结果后，需要调用 `globfree` 函数释放 `glob_t` 结构体中动态分配的资源，以避免内存泄漏。
- 对于更复杂的匹配需求，可以使用 `[...]` 和 `[!...]` 来指定字符集和反字符集。

通过 `glob` 函数，可以方便地进行文件名的通配符匹配，例如在文件操作或批处理任务中根据特定的文件名模式进行文件的查找和处理。



`glob_t` 是一个结构体类型，在 POSIX 系统上用于处理通配符扩展（globbing）。它定义在 `<glob.h>` 头文件中，并用于存储 `glob` 函数执行后返回的匹配文件路径列表及其相关信息。

**结构体定义**

`glob_t` 结构体的定义如下：

```c
typedef struct {
    size_t   gl_pathc;    /* Count of paths matched by the pattern */
    char   **gl_pathv;    /* List of matched pathnames */
    size_t   gl_offs;     /* Slots to reserve in gl_pathv */
    int      gl_flags;    /* Set to zero, no flags are supported */
    void    *gl_reserved; /* Reserved for future use */
} glob_t;
```

- `gl_pathc`: 匹配到的路径数量。
- `gl_pathv`: 包含匹配路径名的字符串数组。
- `gl_offs`: 保留在 `gl_pathv` 中的空槽位数量。
- `gl_flags`: 标志，目前总是为零，不支持任何标志。
- `gl_reserved`: 保留字段，用于未来扩展。

**使用 `glob_t`**

使用 `glob_t` 结构体通常涉及以下步骤：

1. **初始化 `glob_t` 结构体**：在使用之前，需要将 `glob_t` 结构体初始化为零或使用 `glob` 函数初始化。

2. **调用 `glob` 函数**：调用 `glob` 函数执行通配符扩展，该函数填充 `glob_t` 结构体。

3. **处理匹配结果**：检查 `gl_pathc` 确定匹配的路径数量，使用 `gl_pathv` 访问匹配的路径名。

4. **清理资源**：使用完 `glob_t` 结构体后，需要调用 `globfree` 函数释放动态分配的资源。

**示例代码**

以下是一个简单的示例，演示如何使用 `glob_t` 结构体和 `glob` 函数进行通配符匹配：

```c
#include <stdio.h>
#include <glob.h>

int main() {
    glob_t glob_result;
    int ret;

    // 使用 glob 函数匹配当前目录下所有 .txt 文件
    ret = glob("*.txt", 0, NULL, &glob_result);
    if (ret != 0) {
        printf("Failed to glob\n");
        return 1;
    }

    // 输出匹配到的文件路径
    printf("Matched %lu files:\n", (unsigned long)glob_result.gl_pathc);
    for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
        printf("%s\n", glob_result.gl_pathv[i]);
    }

    // 释放 glob_result 中动态分配的资源
    globfree(&glob_result);

    return 0;
}
```

**注意事项**

- 使用 `glob` 函数前要确保 `glob_t` 结构体已经初始化。
- `glob` 函数的第一个参数是匹配模式，第二个参数是标志（通常为 0），第三个参数是一个函数指针，可以用来指定错误处理函数（通常传入 NULL 表示使用默认错误处理），最后一个参数是 `glob_t` 结构体的地址。
- 使用完 `glob_t` 结构体后，必须调用 `globfree` 函数释放资源，防止内存泄漏。

通过 `glob_t` 结构体和 `glob` 函数，可以方便地在 POSIX 系统上进行文件名的通配符匹配和扩展。





`strdup` 是一个非标准但广泛支持的函数，用于在 C 语言中复制字符串。它会动态地分配足够的内存来存储复制后的字符串，并返回指向新分配内存的指针。通常情况下，它的功能等同于先使用 `strlen` 确定字符串长度，然后使用 `malloc` 分配相同长度的内存，最后使用 `strcpy` 复制字符串。

**函数原型**

```c
#include <string.h>

char *strdup(const char *s);
```

- `s`: 要复制的字符串的源，必须是以 null 结尾的 C 字符串。

**返回值**

- 如果分配成功，`strdup` 返回一个指向新分配内存的指针，该内存包含复制的字符串内容，并以 null 结尾。
- 如果内存分配失败，返回 `NULL`。

**使用示例**

以下是一个简单的示例，演示如何使用 `strdup` 函数复制字符串：

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    const char *original = "Hello, strdup!";
    char *copy = strdup(original);

    if (copy != NULL) {
        printf("Original string: %s\n", original);
        printf("Copied string: %s\n", copy);
        
        // 释放复制的字符串的内存
        free(copy);
    } else {
        perror("strdup");
        return 1;
    }

    return 0;
}
```

**注意事项**

- **内存分配**: `strdup` 内部使用 `malloc` 进行内存分配。因此，使用完返回的字符串后，应该使用 `free` 函数释放内存，以避免内存泄漏。

- **错误处理**: 如果 `strdup` 分配内存失败，它会返回 `NULL`。因此，在使用返回值之前，应该检查 `NULL` 指针。

- **非标准函数**: 尽管 `strdup` 在许多系统和编译器中都受支持，但它并不是 ANSI C 或 POSIX 的一部分。如果需要在标准 C 库中使用，请考虑使用标准库函数 `strlen`, `malloc` 和 `strcpy` 来手动实现类似的功能。

`strdup` 是一个方便的函数，特别是在需要复制字符串并处理动态内存分配的情况下。





`fgets` 是 C 标准库中用于从文件流中读取一行字符串的函数。它是比较安全和常用的函数之一，特别适合读取文本文件中的内容。

**函数原型**

```c
#include <stdio.h>

char *fgets(char *str, int size, FILE *stream);
```

- `str`: 指向字符数组的指针，用于存储读取的字符串。该数组必须足够大，可以容纳最多 `size-1` 个字符，因为 `fgets` 会在末尾自动添加 null 字符 `\0`。

- `size`: 最大读取字符数，包括最后的 null 字符 `\0`。通常应该设置为数组 `str` 的大小，以防止缓冲区溢出。

- `stream`: 指向文件流的指针，指定从哪个文件流中读取数据。常见的是使用 `stdin`（标准输入），也可以是已打开的文件指针。

**返回值**

- 如果成功读取到数据，返回 `str` 的指针。
- 如果到达文件末尾或者发生错误，返回 `NULL`。可以通过 `feof` 和 `ferror` 函数来判断是哪种情况。

**使用示例**

以下是一个简单的示例，演示如何使用 `fgets` 函数从标准输入（键盘）读取一行字符串：

```c
#include <stdio.h>

int main() {
    char buffer[100];  // 创建一个足够大的字符数组来存储输入的字符串

    printf("Enter a string: ");
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        printf("You entered: %s", buffer);  // 输出读取到的字符串
    } else {
        perror("fgets");  // 如果读取失败，输出错误信息
        return 1;
    }

    return 0;
}
```

**注意事项**

- **输入处理**: `fgets` 会将输入的换行符 `\n` 保留在字符串中，因此如果不需要换行符，需要手动处理。

- **缓冲区大小**: 确保字符数组 `buffer` 的大小足够大，以便能够容纳预期的输入字符数及最后的 null 字符 `\0`。

- **错误处理**: 检查 `fgets` 的返回值是否为 `NULL`，以确定是否发生了错误或者到达了文件末尾。

- **文件流**: `fgets` 不仅限于标准输入，也可以用于从已打开的文件中读取行。只需将 `stream` 参数设置为指向该文件的文件指针即可。

`fgets` 是一个简单而实用的函数，适用于许多基本的文本输入任务，如从用户那里读取输入或从文件中逐行读取内容。





`pread` 是一个 POSIX 标准的函数，用于从文件描述符指定的位置读取数据。与 `read` 不同，`pread` 在读取过程中不会改变文件偏移量。这使得 `pread` 在并发环境中尤其有用，因为它可以避免由于多个线程或进程共享文件描述符而产生的竞争条件。

**函数原型**

```c
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
```

- `fd`: 文件描述符，从中读取数据。
- `buf`: 指向用于存放读取数据的缓冲区。
- `count`: 要读取的字节数。
- `offset`: 从文件开始的偏移量，指定读取数据的位置。

**返回值**

- 成功时，返回读取的字节数。如果返回值为 0，表示到达文件末尾。
- 失败时，返回 -1，并设置 `errno` 以指示错误类型。

**使用示例**

以下示例展示如何使用 `pread` 从一个文件的特定位置读取数据：

```c
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd = open("example.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 假设要从文件的偏移量 10 处读取 20 个字节
    off_t offset = 10;
    size_t count = 20;
    char buffer[21];  // 多分配一个字节用于 null 终止符

    ssize_t bytesRead = pread(fd, buffer, count, offset);
    if (bytesRead == -1) {
        perror("pread");
        close(fd);
        return 1;
    }

    // 确保缓冲区是以 null 终止的字符串
    buffer[bytesRead] = '\0';
    printf("Read %zd bytes: %s\n", bytesRead, buffer);

    close(fd);
    return 0;
}
```

**注意事项**

1. **文件偏移量**: `pread` 使用的 `offset` 是从文件开始的绝对偏移量，而不是相对于当前文件位置的偏移量。

2. **多线程环境**: `pread` 非常适合在多线程环境中使用，因为它不会改变文件描述符的文件偏移量，避免了并发读写文件时的竞争条件。

3. **返回值检查**: 在使用 `pread` 后，应检查返回值以确保读取操作成功。如果返回值为 -1，应检查 `errno` 以确定错误原因。

4. **性能考虑**: 与 `lseek` 和 `read` 的组合相比，`pread` 可能具有更高的性能，因为它避免了两次系统调用。

5. **字节数**: `pread` 并不一定会读取请求的 `count` 字节数，它可能会返回小于 `count` 的值，特别是在读取文件末尾时。因此，代码应能够处理这种情况。

通过以上信息和示例，可以更好地理解和使用 `pread` 函数来从文件的特定位置读取数据。



## 8、节目单线程



## 9、频道线程

`sched_yield` 是一个 POSIX 标准中定义的函数，用于将当前线程放弃处理器，使得系统可以将处理器时间片分配给其他具有相同优先级的线程。它的作用是让出 CPU 资源，以便其他线程有机会执行。

具体来说，`sched_yield` 函数的功能是：

- 让当前线程放弃 CPU 控制权；
- 尝试让同优先级的其他线程执行；
- 如果没有其他线程需要执行，当前线程可能会立即重新获得 CPU。

在多线程编程中，`sched_yield` 可以用来提高线程的公平性和效率，尤其是在同一优先级的线程竞争 CPU 资源时，通过周期性调用 `sched_yield` 可以避免某个线程长期占用 CPU 而导致其他线程饥饿的情况。

在使用 `sched_yield` 时需要注意：

- 它并不保证当前线程会立即放弃 CPU，具体行为取决于操作系统的调度策略；
- 过度使用 `sched_yield` 可能会导致性能下降，因为频繁的线程切换本身会带来开销；
- 它一般用于在某些特定条件下主动让出 CPU，而不是用于线程的主动休眠或等待。

总结来说，`sched_yield` 是一个线程调度相关的函数，用于提高多线程程序的响应性和公平性。





## 10、项目总结及拓展

要在最顶层的 `Makefile` 中编译到 `src/client` 和 `src/server` 目录下的 `Makefile`，你可以按照以下方式来设置。

假设你的目录结构如下：

```
.
├── doc/
├── INSTALL
├── LISENCE
├── Makefile  (顶层 Makefile)
├── media/
│   ├── ch1/
│   ├── ch2/
│   └── ch3/
├── producer_consumer/
├── producer_consumer.c
├── README
├── README.md
└── src/
    ├── client/
    │   ├── client
    │   ├── client.c
    │   ├── client.h
    │   ├── client.o
    │   └── Makefile
    ├── include/
    │   └── proto.h
    └── server/
        ├── list.txt
        ├── Makefile
        ├── medialib.c
        ├── medialib.h
        ├── medialib.o
        ├── mytbf.c
        ├── mytbf.h
        ├── mytbf.o
        ├── server
        ├── server.c
        ├── server_conf.h
        ├── server.o
        ├── thr_channel.c
        ├── thr_channel.h
        ├── thr_channel.o
        ├── thr_list.c
        ├── thr_list.h
        └── thr_list.o
```

以下是顶层 `Makefile` 的示例，用于递归编译 `src/client` 和 `src/server` 目录下的 `Makefile`：

```makefile
# 定义需要递归编译的子目录列表
SUBDIRS := src/client src/server

# 定义默认目标，递归调用所有子目录的 Makefile
all:
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir; \
    done

# 清理所有子目录的 Makefile 中生成的目标文件和可执行文件
clean:
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir clean; \
    done

# 如果需要其他操作，可以继续定义其他目标
# 例如执行测试或者安装等操作
test:
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir test; \
    done
```

在这个 `Makefile` 中：

- `SUBDIRS := src/client src/server` 定义了需要递归处理的子目录列表，即 `src/client` 和 `src/server`。
- `all` 目标通过 `for` 循环遍历 `SUBDIRS` 中的每个子目录，并使用 `$(MAKE) -C $$dir` 来递归调用每个子目录下的 `Makefile` 进行编译。
- `clean` 和 `test` 目标同样使用 `for` 循环递归调用每个子目录下的 `Makefile` 来执行清理和测试操作。

使用这个顶层的 `Makefile`，你可以方便地管理和编译 `src/client` 和 `src/server` 目录中的代码，同时保持项目的结构清晰和管理简便。



如果你希望在顶层的 `Makefile` 中顺带编译 `producer_consumer.c` 文件，可以在 `Makefile` 中添加相应的目标和规则。

假设你的 `producer_consumer.c` 文件位于顶层目录，你可以将其包含在 `all` 和 `clean` 目标中。以下是更新后的 `Makefile` 示例：

```makefile
# 定义需要递归编译的子目录列表
SUBDIRS := src/client src/server

# 定义默认目标，递归调用所有子目录的 Makefile，并编译顶层文件
all: producer_consumer
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir; \
    done

# 编译顶层的 producer_consumer.c 文件
producer_consumer: producer_consumer.c
    gcc $^ -o $@

# 清理所有子目录的 Makefile 中生成的目标文件和可执行文件，以及顶层编译生成的文件
clean:
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir clean; \
    done
    rm -f producer_consumer

# 如果需要其他操作，可以继续定义其他目标
# 例如执行测试或者安装等操作
test:
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir test; \
    done
```

在这个更新后的 `Makefile` 中：

- 新增了 `producer_consumer` 目标，它依赖于 `producer_consumer.c` 文件，并通过 `gcc` 编译生成可执行文件 `producer_consumer`。
- 在 `all` 目标中，将 `producer_consumer` 添加为依赖项，这样在执行 `make all` 时会顺带编译 `producer_consumer.c` 文件。

确保 `producer_consumer.c` 文件位于顶层目录，并且其内容包含了正确的 C 代码。使用这个更新后的 `Makefile`，你可以同时管理和编译顶层的 `producer_consumer.c` 文件以及 `src/client` 和 `src/server` 目录中的代码。



如果你希望能够在修改代码后仅通过 `make` 命令进行增量编译，而不必每次都执行 `make clean` 删除所有目标文件和可执行文件，可以通过正确设置依赖关系和使用 Makefile 的规则来实现。

通常情况下，Makefile 会根据源文件和目标文件的时间戳来判断是否需要重新编译。只有当源文件更新或者某个目标文件不存在时，才会重新执行编译动作。这样可以避免每次都完全重新编译整个项目。

以下是一个示例的 Makefile，展示如何设置依赖关系和规则来实现增量编译：

```makefile
# 定义需要递归编译的子目录列表
SUBDIRS := src/client src/server

# 目标：默认目标为编译所有子目录和顶层文件
all: producer_consumer
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir; \
    done

# 目标：编译顶层的 producer_consumer.c 文件
producer_consumer: producer_consumer.c
    gcc $^ -o $@

# 清理目标：清理所有子目录的 Makefile 中生成的目标文件和可执行文件，以及顶层编译生成的文件
clean:
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir clean; \
    done
    rm -f producer_consumer

# 如果需要其他操作，可以继续定义其他目标
# 例如执行测试或者安装等操作
test:
    @for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir test; \
    done
```

**关键点解释：**

1. **默认目标 (`all`)：**
   - `all` 目标依赖于 `producer_consumer` 目标和 `SUBDIRS` 中的子目录。
   - 当执行 `make all` 时，会先编译 `producer_consumer.c` 生成 `producer_consumer` 可执行文件，然后递归调用每个子目录的 `Makefile` 进行编译。

2. **producer_consumer 目标：**
   - `producer_consumer` 目标定义了 `producer_consumer.c` 文件的编译规则。
   - 使用 `gcc` 编译 `producer_consumer.c` 文件生成 `producer_consumer` 可执行文件。

3. **增量编译：**
   - Make 工具会根据文件的时间戳来决定是否需要重新编译。只有当源文件或者目标文件的时间戳发生变化时，才会重新执行编译动作。
   - 这意味着，如果你修改了某个源文件（比如 `producer_consumer.c` 或者任何 `src/client` 和 `src/server` 中的源文件），然后再次执行 `make all`，Make 工具会自动检测到变化，只重新编译有修改的部分，而不是整个项目。

通过正确设置依赖关系和使用增量编译的方式，你可以避免每次都需要执行 `make clean` 来删除所有目标文件和可执行文件，从而提高编译效率。