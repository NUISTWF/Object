#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int level; // 树的阶数
    int keyNum; // 关键字数量
    int childNum;
    int* keys; // 关键字数组
    struct Node* parent; // 父亲指针
    struct Node** children; // 孩子指针数组
} Node;

// 初始化结点
Node* initNode(int level) {
    Node* node = (Node*)malloc(sizeof(Node));
    node -> level = level;
    node -> keyNum = 0;
    node -> childNum = 0;
    node -> parent = NULL;
    node -> keys = (int*)malloc(sizeof(int) * (level + 1));
    node -> children = (Node**)malloc(sizeof(Node*) * level);
    for (int i = 0; i < level; i++) {
        node -> keys[i] = 0;
        node -> children[i] = NULL;
    }
    return node;
}

// 从结点中找到合适的插入位置
int findSuiteIndex(Node* node, int data) {
    int index;
    for (index = 1; index <= node -> keyNum; index++) {
         if (data < node -> keys[index])
             break;
    }
    return index;
}

// 往结点中插入数据
void addData(Node* node, int data, Node** T) {
    int index = findSuiteIndex(node, data);
    for (int i = node -> keyNum; i >= index; i--) {
        node -> keys[i + 1] = node -> keys[i]; 
    }
    node -> keys[index] = data;
    node -> keyNum = node -> keyNum + 1;
    if (node -> keyNum == node -> level) {
        // 开始分裂，找到中间位置
        int mid = node -> level / 2 + node -> level % 2;
        // 初始化左孩子节点
        Node* lchild = initNode(node -> level);
        // 初始化有孩子节点
        Node* rchild = initNode(node -> level);
        // 将mid左边的值赋值给左孩子
        for (int i = 1; i < mid; i++) {
            addData(lchild, node -> keys[i], T);
        }
        // 将mid右边的值赋值给右孩子
        for (int i = mid + 1; i <= node -> keyNum; i++) {
            addData(rchild, node -> keys[i], T);
        }
        // 将原先节点mid左边的孩子赋值给分裂出来的左孩子
        for (int i = 0; i < mid; i++) {
            lchild -> children[i] = node -> children[i];
            if (node -> children[i] != NULL) {
                node -> children[i] -> parent = lchild;
                lchild -> childNum ++;
            }
        }
        // 将原先节点mid右边的孩子赋值给分裂出来的右孩子
        int index = 0;
        for (int i = mid; i < node -> childNum; i++) {
            rchild -> children[index++] = node -> children[i];
            if (node -> children[i] != NULL) {
                node -> children[i] -> parent = rchild;
                rchild -> childNum ++;
            }
        }
        //判断当前节点是不是根节点
        if (node -> parent == NULL) {
            // 是根节点
            Node* newParent = initNode(node -> level);
            addData(newParent, node -> keys[mid], T);
            newParent -> children[0] = lchild;
            newParent -> children[1] = rchild;
            newParent -> childNum = 2;
            lchild -> parent = newParent;
            rchild -> parent = newParent;
            *T = newParent;
        }
        else {
            // 不是根节点
            int index = findSuiteIndex(node -> parent, node -> keys[mid]);
            lchild -> parent = node -> parent;
            rchild -> parent = node -> parent;
            node -> parent -> children[index - 1] = lchild;
            if (node -> parent -> children[index] != NULL) {
                for (int i = node -> parent -> childNum - 1; i >= index; i--) {
                    node -> parent -> children[i + 1] = node -> parent -> children[i];
                }
            }
            node -> parent -> children[index] = rchild;
            node -> parent -> childNum ++;
            addData(node -> parent, node -> keys[mid], T);
        }
    }
}

Node* findSuiteLeafNode(Node* T, int data) {
    if (T -> childNum == 0)
        return T;
    else {
        int index = findSuiteIndex(T, data);
        return findSuiteLeafNode(T -> children[index - 1], data);
    }
}

Node* find(Node* node, int data) {
    if (node == NULL) {
        return NULL;
    }
    for (int i = 1; i <= node -> keyNum; i++) {
        if (data == node -> keys[i]) {
            return node;
        }
        else if (data < node -> keys[i]) {
            return find(node -> children[i - 1], data);
        }
        else {
            if (i != node -> keyNum && data < node -> keys[i + 1]) 
                return find(node -> children[i], data);
        }
    }
    return find(node -> children[node -> keyNum], data);
}

// 插入结点
void insert(Node** T, int data) {
    Node* node = findSuiteLeafNode(*T, data);
    addData(node, data, T);
}

void printTree(Node* T) {
    if (T != NULL) {
        for (int i  = 1; i <= T -> keyNum; i++) {
            printf("%d ", T -> keys[i]);
        }
        printf("\n");
        for (int i = 0; i < T -> childNum; i++) {
            printTree(T -> children[i]);
        }
    }
} 

int main() {
    Node* T = initNode(5);
    insert(&T, 1);
    insert(&T, 2);
    insert(&T, 6);
    insert(&T, 7);
    insert(&T, 11);
    insert(&T, 4);
    insert(&T, 8);
    insert(&T, 13);
    insert(&T, 10);
    insert(&T, 5);
    insert(&T, 17);
    insert(&T, 9);
    insert(&T, 16);
    insert(&T, 20);
    insert(&T, 3);
    insert(&T, 12);
    insert(&T, 14);
    insert(&T, 18);
    insert(&T, 19);
    insert(&T, 15);
    printTree(T);
    Node* node = find(T, 7);
    if (node) {
        for (int i = 1; i <= node -> keyNum; i++) {
            printf("%d ", node -> keys[i]);
        }
        printf("\n");
    }
    return 0;
}
