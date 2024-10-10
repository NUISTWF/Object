/* 搜索树 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_SIZE    128
#define DESC_SIZE   128
#define BUF_SIZE    KEY_SIZE + DESC_SIZE
#define FILENAME    "dictionary.txt"

typedef struct treeNode {
    struct treeNode* ch[26];        // 26个字母指针
    char desc[DESC_SIZE];           // 当前代表的内容
} Node;

int getWord(FILE* fp, char* key, char* desc)
{
    int i = 0;
    int j = 0;
    char buf[BUF_SIZE];

    if (fgets(buf, BUF_SIZE, fp) == NULL)
        return -1;
    
    for (i = 0; (i < KEY_SIZE - 1) && (buf[i] != ':'); ++i) {
        key[i] = buf[i];
    }
    key[i] = '\0';
    i++;
    for (j = 0; (j < DESC_SIZE - 1) && (buf[i] != '\0'); ++i, ++j) {
        desc[j] = buf[i];
    }
    desc[j] = '\0';
    return 0;
}

void insertTree(Node** tree, char* key, char* desc)
{
    Node* node = NULL;
    if (NULL == *tree) {
        node = malloc(sizeof(Node));
        for (int i = 0; i < 26; ++i)
            node->ch[i] = NULL;
        node->desc[0] = '\0';
        *tree = node;
    }
    if (*key == '\0') {
        strcpy((*tree)->desc, desc);
        return ;
    }
    insertTree(&((*tree)->ch[*key - 'a']), key + 1, desc);
}

Node* findNode(Node* tree, char* key)
{
    if (NULL == tree)
        return NULL;
    if (*key == '\0') {
        return tree;
    }
    return findNode(tree->ch[*key - 'a'], key + 1);
}

int main(int argc, char** argv)
{
    Node* tree = NULL;
    FILE* fp = NULL;
    char key[KEY_SIZE] = {'\0'};
    char desc[DESC_SIZE] = {'\0'};
    Node* node = NULL;

    if (argc < 2) {
        printf("./retrieval ant/buffterful/cobra/donkey\n");
		exit(1);
    }

    fp = fopen(FILENAME, "r");

    while (1) {
        if (getWord(fp, key, desc) != 0)
           break;
        insertTree(&tree, key, desc);
    }
    printf("%s", findNode(tree, argv[1])->desc);

    fclose(fp);
    return 0;
}
