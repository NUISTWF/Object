/* 字符串暴力匹配 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct StringNode {
    char* data;
    int len;
} String;

String* initString()
{
    String* s = malloc(sizeof(String));
    s->data = NULL;
    s->len = 0;
    return s;
}

void stringAssign(String* s, char* data)
{
    if (s->data)            // 这个字符串结构体内数据域不能有值，有值就需要free，下面要开辟内存空间存放字符串
        free(s->data);
    int len = 0;
    char* temp = data;
    while (*temp) {
        len++;              // 获取字符串的长度
        temp++;
    }
    if (len == 0) {         // 如果len为0则传进来的数据为空
        s->data = NULL;
        s->len = 0;
    }
    temp = data;
    s->len = len;           // s->len = strlen(data);  strcpy(s->data, data);
    s->data = malloc(sizeof(char) * (len + 1));
    for (int i = 0; i < len; ++i) {         // 传进来有效字符串就赋值给数据域指针
        s->data[i] = *temp;
        temp++;
    }
    s->data[len] = '\0';
}

void printString(String* s)
{
    for (int i = 0; i < (s->len); ++i) {
        printf((i == 0) ? "%c " : "-> %c ", s->data[i]);
    }
    printf("\ndata = %s and len = %ld\n", s->data, strlen(s->data));
}

void forceMatch(String* master, String* substring)
{
    int i = 0;              // 两个指针指向主从字符串的遍历位置
    int j = 0;              // 暴力匹配就是主串第一个开始比较，一旦跟子串不同就回到起始的下一个开始重新与子串的头开始比较
    while ((i < master->len) && (j < substring->len)) {
        if (master->data[i] == substring->data[j]) {
            i++;
            j++;
        } else {
            i = i - j + 1;  // 主串每次从此次比较的后面一个开始比对
            j = 0;          // 子串每次从头开始比对
        }
    }
    if (j == substring->len) {
        printf("force match success.\n");
    } else if (i == master->len) {
        printf("force match fail.\n");
    }
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("./string xiaomo mo\n");
        return 0;
    }
    String* masterString = initString();
    stringAssign(masterString, argv[1]);
    String* subString = initString();
    stringAssign(subString, argv[2]);
    printString(masterString);
    printString(subString);    
    forceMatch(masterString, subString);
    return 0;
}