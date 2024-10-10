/* kmp算法 */

#include <stdio.h>
#include <stdlib.h>

typedef struct StringNode {
    char* data;
    int len;
} String;

String* initString()
{
    String* s = malloc(sizeof(String));
    s->data = NULL;
    s->len = 0;
}

void stringAssign(String* s, char* data)
{
    if (s->data)
        free(s->data);
    char* temp = data;
    int len = 0;
    while (*temp) {
        temp++;
        len++;
    }
    if (!len) {
        s->data = NULL;
        s->len = 0;
    }
    temp = data;
    s->len = len;
    s->data = malloc(sizeof(char) * (len + 1));
    for (int i = 0; i < len; ++i) {
        s->data[i] = *temp;
        temp++;
    }
    s->data[len] = '\0';
}

void printString(String* s)
{
    for (int i = 0; i < (s->len); ++i)
        printf("%c ", s->data[i]);
    printf("\n");
}

void printNext(int* next, int len)
{
    for (int i = 0; i < len; ++i)
        printf("%d ", next[i]);
    printf("\n");
}

int* getNext(String* pattern)
{
    int* next = malloc(sizeof(int) * pattern->len);
    int i = 0;
    int j = -1;
    next[0] = -1;
    while (i < pattern->len - 1) {
        if (j == -1 || pattern->data[i] == pattern->data[j]) {
            next[++i] = ++j;
        } else {
            j = next[j];
        }
    }
    return next;
}

void kmpSearch(String* text, String* pattern)
{
    int *next = getNext(pattern);
    int i = 0;
    int j = 0;
    while (i < text->len && j < pattern->len) {
        if (j == -1 || text->data[i] == pattern->data[j]) {
            ++i;
            ++j;
        } else {
            j = next[j];
        }
    }
    if (j == pattern->len) {
        printf("kmp match success.\n");
    } else {
        printf("kmp match fail.\n");
    }
}

int main()
{
    String* text = initString();
    stringAssign(text, "ababbca");
    String* pattern = initString();
    stringAssign(pattern, "bbc");
    printString(text);
    printString(pattern);
    int* next = getNext(pattern);
    printNext(next, pattern->len);
    kmpSearch(text, pattern);
    return 0;
}
