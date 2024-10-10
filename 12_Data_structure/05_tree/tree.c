/* 二叉树
 * 层次遍历（队列），非递归前中后序遍历（栈，入左出右），二叉搜索树前中后序遍历
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TreeNode {
    char data;
	struct TreeNode *left;
    struct TreeNode *right;
    struct TreeNode *parent;
    int flag;
    int ltag;
    int rtag;
} Node;

typedef struct QueueNode {
    Node* nodeData;
    struct QueueNode* next;
} Queue;

typedef struct StackNode {
    Node* nodeData;
    struct StackNode* next;
} Stack;

// 入栈
void push(Stack** stack, Node* data)
{
    Stack* node = NULL;
    node = malloc(sizeof(Stack));
    node->nodeData = data;
    node->next = *stack;
    *stack = node;
}

// 出栈
int pop(Stack** stack, Node** node)
{
    if (NULL == *stack) {
        return -1;
    }
    Stack* cur = *stack;
    *node = cur->nodeData;
    *stack = cur->next;
}

// 打印栈
void printStack(Stack* stack)
{
    Stack* cur = stack;
    while (cur != NULL) {
        printf("%c ", cur->nodeData->data);
        cur = cur->next;
    }
}

// 非递归的前序遍历（前序遍历，入栈记录）
void nonPreorderTraversal(Node* tree)
{
    Stack* stack = NULL;
    Node* cur = tree;
    while (1) {                                 // 不断循环的去入栈出栈
        if (cur != NULL) {                      // 左边不为空就入栈
            push(&stack, cur);
            printf("%c ", cur->data);           // 先序入栈就打印
            cur = cur->left;
        } else {
            if (pop(&stack, &cur) == -1)        // 当左边为空就开始出栈
                break;                          // 栈空了就退出循环结束遍历
            cur = cur->right;
        }
    }
}

// 非递归的中序遍历（中续遍历，出栈记录）
void nonMidTraversal(Node* tree)
{
    Stack* stack = NULL;
    Node* cur = tree;
    while (1) {
        if (cur != NULL) {
            push(&stack, cur);
            cur = cur->left;
        } else {
            if (pop(&stack, &cur) == -1)        // 当左边为空就开始出栈
                break;
            printf("%c ", cur->data);           // 中序出栈再打印
            cur = cur->right;
        }
    }
}

Node* getStackTop(Stack* stack)
{
    if (stack == NULL)
        return NULL;
    return stack->nodeData;
}

// 非递归的后序遍历（后续遍历，需要注意出栈时判断有无右子树，如果入过栈就pass掉）
void nonAfterTraversal(Node* tree)
{
    Stack* stack = NULL;
    Node* cur = tree;
    Node* top = NULL;
    Node* tmp = NULL;
    while (1) {
        if (cur != NULL) {
            cur->flag = 1;                      // 入栈前标记一下，该节点入过栈了
            push(&stack, cur);
            cur = cur->left;
        } else {
            top = getStackTop(stack);           // 获取栈顶元素（我知道为什么段错误了，因为最后一次结束栈空了，再获取这个元素就出错）
            if (top == NULL)
                break;
            if ((top->right != NULL) && (top->right->flag == 0)) {
                cur = top->right;               // 有右孩子就先右节点入栈再出栈
            } else {                            // 无右节点则出栈
                pop(&stack, &tmp);              // 出栈之前需要判断有无右子树，且右子树没有被入栈过；如果有右子树就入栈右子树
                printf("%c ", tmp->data);       // 后序出栈再打印
            }
        } 
    }
}

// 入队（队列里面每一项存的都是树的节点数据，即char的字符）
void enQueue(Queue** queue, Node* data)
{
    Queue* node = NULL;
    Queue* cur = *queue;
    if (NULL == *queue) {
        node = malloc(sizeof(Queue));
        node->nodeData = data;
        node->next = NULL;
        *queue = node;
        return ;
    }
    while (cur->next != NULL) {
        cur = cur->next;
    }
    node = malloc(sizeof(Queue));
    node->nodeData = data;
    node->next = NULL;
    cur->next = node;
}

// 出队（无头节点的出队，需要二级指针，因为需要更改头节点到next下一个中去）
int deQueue(Queue** queue, Node** node)
{
    if (NULL == *queue)
        return -1;
    Queue* cur = *queue;
    *node = cur->nodeData;          // 出队的同时保存这个节点
    *queue = cur->next;
}

// 打印队列
void printQueue(Queue* queue)
{
    while (queue != NULL) {
        printf("%c\n", queue->nodeData->data);
        queue = queue->next;
    }
}

// 销毁释放队列
void destroyQueue(Queue* queue)
{
    Queue* cur = queue;
    Queue* tmp = NULL;
    while (cur != NULL) {
        tmp = cur->next;
        printf("%c ", cur->nodeData->data);
        free(cur);
        cur = tmp;
    }
}

// 层次遍历（放入队列中，出队一个，就将他的左右孩子放入队列，再出队下一个，再放入出队节点的左右孩子）
void levelTraversal(Node* tree)
{
    Queue* queue = NULL;
    Node* cur = NULL;
    enQueue(&queue, tree);
    while (1) {
        if (deQueue(&queue, &cur) == -1)
            break;
        printf("%c ", cur->data);
        if (cur->left != NULL)
			enQueue(&queue, cur->left);
		if (cur->right != NULL)
			enQueue(&queue, cur->right);
    }
}

// 利用先序去创建树，# 结束递归
void treeCreate(Node** tree, char* data, int index, Node* parent)
{
    static int i = 0;   // 静态变量i用来依次指向数组的下一个内容（每次进来不会变）
    char ch = data[i++];
    // 我这个index索引也在不断地变化，同一个treeCreate里创建左子树右子树的index都不一样，用指针一直改变值；要么全局变量或者静态变量
    if (ch == '#') {
        *tree == NULL;
    } else {
        *tree = (Node*)malloc(sizeof(Node));
        (*tree)->data = ch;
        (*tree)->flag = 0;
        (*tree)->ltag = 0;
        (*tree)->rtag = 0;
        (*tree)->parent = parent;
        treeCreate(&((*tree)->left), data, i, *tree);
        treeCreate(&((*tree)->right), data, i, *tree);
    }
}

// BST二叉排序树（小插左，大插右）
void insertBST(Node** tree, char data)
{
    Node* node = NULL;
    if (NULL == *tree) {
        node = malloc(sizeof(Node));
        node->data = data;
        node->left = NULL;
        node->right = NULL;
        *tree = node;
        return ;
    }

    if (data < ((*tree)->data))
        insertBST(&((*tree)->left), data);
    else
        insertBST(&((*tree)->right), data);
}

// BST二叉排序树的查找
Node* findBST(Node* tree, char data)
{
    if (NULL == tree)
        return NULL;
    if (tree->data == data) {
        return tree;
    }
    if (data < tree->data) {
        findBST(tree->left, data);
    } else {
        findBST(tree->right, data);
    }
}

/*===============================================================================*/

// 线索树（中序：左根右）
void midThreadTree(Node* tree, Node** prev)     // prev代表前驱节点（二级指针是因为需要它的值一直在变化；因为递归是需要返回上一层的，不用二级指针，值传递，怎么返回值呢）
{
    if (tree != NULL) {
        midThreadTree(tree->left, prev);        // 一直往左边递归进去，直到找到最左下角的节点，操作完该节点，也就是执行完这一个函数又返回上一层的这里开始执行下一个后继节点
        if (tree->left == NULL) {               // 左节点为空则左节点就指向它的前驱
            tree->left = *prev;
            tree->ltag = 1;                     // 代表指向的是前驱后继节点而不是孩子节点
        }
        if (((*prev) != NULL) && ((*prev)->right == NULL)) {  // 这里做的是此时遍历到一个有孩子的节点，则该节点的前驱里面没有右孩子，就将右孩子指向当前节点
            (*prev)->right = tree;              // 前驱的右节点为空，就使它指向当前节点，当前节点是他的后继节点
            (*prev)->rtag = 1;
        }
        *prev = tree;                           // 前驱指针的移动（执行根节点的时候，每次前驱都等于当前的节点）
        midThreadTree(tree->right, prev);       // 上一行将当前的节点赋给前驱指针，从左到右依次遍历
    }
}

// 下面两个函数是为了遍历中序线索二叉树
// 找到第一个节点，也就是最左边的节点
Node* getMidFirst(Node* tree)
{
    while (tree->ltag == 0)         // 直到ltag为1代表有前驱，此时就是第一个节点
        tree = tree->left;
    return tree;
}

// 找到该节点的下一个节点（找下一个就是找右孩子是不是后继节点，如果是右孩子，则找右孩子的最左边节点，因为这个右孩子的最左边节点的前驱是刚刚这个节点，这样就连起来了）
Node* getMidNext(Node* tree)
{
    if (tree->rtag == 1)            // 如果右节点是后继的话就直接返回后继，也就是遍历的下一个节点
        return tree->right;
    return getMidFirst(tree->right);// 如果右节点不是后继，是右孩子的话，则继续找右孩子这棵树的第一个节点，也就是找到该节点最左边的叶子节点，因为是左根右，所以最左边的是上一个节点的后面一个
}

/*===============================================================================*/

// 线索树（先序：根左右）(因为先序在办事之前改变了左子树的指针)
void frontThreadTree(Node* tree, Node** prev)
{
    if (tree != NULL) {
        Node* l = tree->left;               // 先保存左右节点，不然就陷入死循环了，不断地在节点A和节点B之间循环（也可以用下面的方法来判断左节点是孩子还是前驱，前驱的话就不执行了）
        if (tree->left == NULL) {
            tree->left = *prev;
            tree->ltag = 1;
        }
        if (((*prev) != NULL) && ((*prev)->right == NULL)) {
            (*prev)->right = tree;
            (*prev)->rtag = 1;
        }
        *prev = tree;                       // 保存前驱节点
        /*if (tree->ltag == 0)              // 此时该树ltag为0说明有左孩子再进行线索化，如果已经线索化过，则ltag为1有前驱了，就不执行该代码了
            frontThreadTree(tree->left, prev);*/
        frontThreadTree(l, prev);           // 比如ABC就三个节点，此时到B节点时，b的左子树已经是前驱也就是a了，又把b的左子树传入，则就是将a传入，此时陷入死循环
        frontThreadTree(tree->right, prev); // 因为在函数内就改变了tree->left = *prev;的值，右边的值没变，所以不需要动
    }
}

// 下面函数是为了遍历先序线索二叉树
// 先序遍历第一个节点就是根，不需要getFirst，直接找下一个即可
Node* getFrontNext(Node* tree)
{   
    if (tree->rtag == 1) {        // 右子树如果是后继则就是在后面，如果不是后继节点，则前序是根左右，就返回左节点
        return tree->right;
    } else {
        if (tree->ltag == 0)      // 因为右边没有后继节点的话，左孩子不一定是左孩子，还可能是左前驱（ltag=0说明有左孩子）
            return tree->left;
        else                      // 如果左子树有前驱了，则找到它的右子树
            return tree->right;
    }
}

/*===============================================================================*/

// 线索树（后序：左右根）
void afterThreadTree(Node* tree, Node** prev)
{
    if (tree != NULL) {
        afterThreadTree(tree->left, prev);
        afterThreadTree(tree->right, prev);
        if (tree->left == NULL) {
            tree->left = *prev;
            tree->ltag = 1;
        }
        if (((*prev) != NULL) && ((*prev)->right == NULL)) {
            (*prev)->right = tree;
            (*prev)->rtag = 1;
        }
        *prev = tree;
    }
}

// 下面函数是为了遍历后序线索二叉树（左右根）
// 这个函数是找该树的第一个节点，左右根，确保没有左右节点后此时该节点才是第一个节点（最左边且没有右节点）
Node* getAfterFirst(Node* tree)
{
    while (tree->ltag == 0) {
        tree = tree->left;
    }
    if (tree->rtag == 0) {              // 有右子树就拿到右子树再次以该节点为树去查找最左边的节点
        return getAfterFirst(tree->right);
    }                                   
    return tree;                        // 没有右子树此时就是最左边的节点
}

Node* getAfterNext(Node* tree)
{
    if (tree->rtag == 1) {
        return tree->right;
    } else {                                      // 分为三种情况，此节点为最终的根节点；此节点为上层根节点的右节点；此节点为上层根节点的左节点
        if (tree->parent == NULL) {               // 如果父亲节点是空，则此时tree是最大的根节点，此时遍历结束，返回空
            return NULL;
        } else if (tree->parent->right == tree) { // 此时父节点不为空说明上面还有节点，如果该节点是右孩子则返回该节点的父亲节点
            return tree->parent;
        } else if (tree->parent->left == tree) {  // 如果是左孩子，即B的时候，没有任何指向，只能通过父亲节点来走
            if (tree->parent->rtag == 0) {        // 此时父亲节点有右孩子则返回该右子树的首节点
                return getAfterFirst(tree->parent->right); // 因为后序是左右根，走到根且没有前驱后继的时候，此时就是一棵树的根，就往上遍历，找此时父节点的右子树
            } else {                              // 如果父亲节点没有右孩子，则返回父亲节点，意味着该节点结束下一个就是父亲节点
                return tree->parent;
            }
        }
    }
}

/*===============================================================================*/

// 前序递归遍历
void preorderTraversal(Node* tree)
{
    if (NULL == tree)
        return ;
    printf("%c ", tree->data);
    preorderTraversal(tree->left);
    preorderTraversal(tree->right);
}

// 中序递归遍历
void midTraversal(Node* tree)
{
    if (NULL == tree)
        return ;
    midTraversal(tree->left);
    printf("%d ", tree->data);    
    midTraversal(tree->right);
}

// 后序递归遍历
void afterTraversal(Node* tree)
{
    if (NULL == tree)
        return ;
    afterTraversal(tree->left); 
    afterTraversal(tree->right);
    printf("%c ", tree->data);
}

// 画树
void drawTree(Node* tree, int tab)
{
    if (NULL == tree)
        return ;
    drawTree(tree->right, tab + 1);
    for (int i = 0; i < tab; ++i) {
        printf("    ");
    }
    printf("%d\n", tree->data);
    drawTree(tree->left, tab + 1);    
}

// 获取高度，树的深度 = MAX(左子树的深度, 右子树的深度) + 1
int getTreeDeep(Node *tree)
{
    if (tree == NULL)
        return 0;
    int l = getTreeDeep(tree->left);
    int r = getTreeDeep(tree->right);
    return (l > r ? l : r) + 1;
}

// LL
void llRotation(Node** root)
{
    Node* tmp = (*root)->left;
    (*root)->left = tmp->right;
    tmp->right = *root;
    *root = tmp;
}

// RR
void rrRotation(Node** root)
{
    Node* tmp = (*root)->right;
    (*root)->right = tmp->left;
    tmp->left = *root;
    *root = tmp;
}

// AVL树
void insertAVL(Node** tree, char data)
{
    Node* node = NULL;
    if (NULL == *tree) {
        node = malloc(sizeof(Node));
        node->data = data;
        node->left = NULL;
        node->right = NULL;
        *tree = node;
        return ;
    }
    if (data < (*tree)->data) {
        insertAVL(&((*tree)->left), data);
        // 拿到当前节点左右子树的高度
        int sub = getTreeDeep((*tree)->left) - getTreeDeep((*tree)->right);
        if (sub < -1 || sub > 1) {
            if (data < (*tree)->left->data) {
                // LL 调整
                llRotation(tree);
            } else {
                // LR 调整
                rrRotation(&((*tree)->left));
                llRotation(tree);
            }
        }
    } else {
        insertAVL(&((*tree)->right), data);
        int sub = getTreeDeep((*tree)->left) - getTreeDeep((*tree)->right);
        if (sub < -1 || sub > 1) {
            if (data > (*tree)->right->data) {
                // RR 调整
                rrRotation(tree);
            } else {
                // RL 调整
                llRotation((&((*tree)->right)));
                rrRotation(tree); 
            }
        }
    }
}

/*======================================哈夫曼树======================================*/

// 哈夫曼树的数据结构；用 digital 数据域存储一连串的节点链表，还有一个len代表当前里面存储的节点数
typedef struct HuffmanTree{
    Node** digital;   // 等同于 Node* digital[7]; 二级指针就是指针数组，用来动态构建内存（每个指针都指向Node类型的空间）
    int len;
} Huffman;

// 初始化哈夫曼树（就是创建好数据结构，将初始值放入）
Huffman* initHuffmanTree(int* weight, int length)
{
    Huffman* tree = (Huffman*)malloc(sizeof(Huffman));
    tree->len = length;
    tree->digital = (Node**)malloc(sizeof(Node*) * (2 * length - 1));   // digital数组里每一项都是Node*类型的指针
    // 上面这一行是分配了哈夫曼这个指针数组的内存空间，下面为每个指针分配内存空间
    for (int i = 0; i < (2 * length - 1); ++i) {
        tree->digital[i] = (Node*)malloc(sizeof(Node)); // 指针指向的每一块内存都是Node类型的数据
    }
    for (int i = 0; i < length; ++i) {                  // 这是对已有数据的初始化，还有len-1个数据域没有初始化
        tree->digital[i]->data = weight[i];
        tree->digital[i]->left = NULL;
        tree->digital[i]->right = NULL;
        tree->digital[i]->parent = NULL;

    }
    for (int i = length; i < (2 * length - 1); ++i) {
        tree->digital[i]->data = 0;
        tree->digital[i]->left = NULL;
        tree->digital[i]->right = NULL;
        tree->digital[i]->parent = NULL;
    }
    return tree;
}

// 获取节点中data权值最小的两个节点（两个节点存在指针数组中，一个数组值存的是最小节点的指针，另外一个是第二小的节点指针）
Node** getSmallestTwoNodes(Huffman* tree)
{
    Node* firstMin = NULL;
    Node* secondMin = NULL;
    int min1 = 65535;                            // 假设一个最大值
    int min2 = 65535;
    int index = 0;
    // 找第一小的
    for (int i = 0; i < tree->len; ++i) {
        if (tree->digital[i]->parent == NULL) {  // 只有该节点的父亲节点为空的时候才是需要找的节点
            if (tree->digital[i]->data < min1) {
                min1 = tree->digital[i]->data;
                firstMin = tree->digital[i];
                index = i;
            }
        }
    }
    // 找第二小的（遍历中，还要去除掉已经找到的第一小的）
    for (int i = 0; i < tree->len; ++i) {
        if ((tree->digital[i]->parent == NULL) && (i != index)) {
            if (tree->digital[i]->data < min2) {
                min2 = tree->digital[i]->data;
                secondMin = tree->digital[i];
            }
        }
    }
    Node** res = (Node**)malloc(sizeof(Node*) * 2);
    res[0] = firstMin;
    res[1] = secondMin;
    return res;         // 想返回两个指针，就搞一个指针数组装起来返回即可
}

// 创建哈夫曼树（找到数组列表中最小的两个，然后将他们的父亲节点指向新空间，将他们连接起来）
void createHuffmanTree(Huffman* tree)
{
    Node** res = NULL;
    Node* firstMin = NULL;
    Node* secondMin = NULL;
    int length = tree->len * 2 - 1;
    for (int i = tree->len; i < length; ++i) {      // 这也就是循环将空余的节点都与初始节点接上
        res = getSmallestTwoNodes(tree);
        firstMin = res[0];
        secondMin = res[1];
        tree->digital[i]->data = firstMin->data + secondMin->data;
        tree->digital[i]->parent = NULL;
        tree->digital[i]->left = firstMin;          // 该父亲节点的左孩子是两个节点中较小的
        tree->digital[i]->right = secondMin;        // (firstMin->data > secondMin->data ? firstMin : secondMin); 要是两个都这么赋值，就导致了data一样大的时候，该节点指向的左右子树是同一个节点
        firstMin->parent = tree->digital[i];
        secondMin->parent = tree->digital[i];
        (tree->len)++;
    }
}

/*======================================图======================================*/
// 图的数据结构（哪些顶点，矩阵列表代表有无边，顶点数，边数）
#define MAX 1234    // MAX 代表顶点之间不通

typedef struct graph {
    char* vexs;     // 图的顶点元素
    int** arcs;     // 图的二维数组
    int vexNum;     // 顶点个数
    int arcNum;     // 边数
} Graph;

Graph* createGraph(int* arc, char* vex, int num)
{
    Graph* graph = malloc(sizeof(Graph));
    graph->vexNum = num;
    graph->arcNum = 0;
    graph->vexs = malloc(sizeof(char) * num);
    for (int i = 0; i < num; ++i) {
        graph->vexs[i] = vex[i];
    }
    graph->arcs = malloc(sizeof(int*) * num);               // 每行都是int*的指针
    for (int i = 0; i < num; ++i) {
        graph->arcs[i] = malloc(sizeof(int) * num);         // 这一行里面有num个int类型的数据
    }
    // 上面全是初始化，下面是创建图填充数据
    for (int i = 0; i < graph->vexNum; ++i) {
        for (int j = 0; j < graph->vexNum; ++j) {
            graph->arcs[i][j] = *(arc + i * graph->vexNum + j);     // 将传进来的数组元素拿出来赋值给二维数组
            if ((graph->arcs[i][j] > 0) && (graph->arcs[i][j] != MAX)) {
                graph->arcNum++;
            }
        }
    }
    // graph->arcNum /= 2;                                     // 因为是无向图，所以除2，有向图就不需要除2
    return graph;
}

/*======================================DFS/BFS======================================*/

void DFS(Graph* graph, int* visited, int index)
{
    printf("%c ", graph->vexs[index]);
    visited[index] = 1;
    for (int i = 0; i < graph->vexNum; ++i) {
        if ((visited[i] != 1) && (graph->arcs[index][i] > 0) && (graph->arcs[index][i] != MAX)) {
            DFS(graph, visited, i);
        }
    }
}

// 专门为BFS广度优先遍历创建的队列
typedef struct BFSQueue {
    int BFSData;
    struct BFSQueue* next;
} BFSqueue;

void BFS_enqueue(BFSqueue** queue, int data)
{
    BFSqueue* node = NULL;
    BFSqueue* cur = *queue;
    if (NULL == *queue) {
        node = malloc(sizeof(BFSqueue));
        node->BFSData = data;
        node->next = NULL;
        *queue = node;
        return ;
    }
    while (cur->next != NULL) {
        cur = cur->next;
    }
    node = malloc(sizeof(BFSqueue));
    node->BFSData = data;
    node->next = NULL;
    cur->next = node;
}

int BFS_dequeue(BFSqueue** queue, int* node)
{
    if (NULL == *queue)
        return -1;
    BFSqueue* cur = *queue;
    *node = cur->BFSData;
    *queue = cur->next;
}

void BFS(Graph* graph, int* visited, int index)
{
    BFSqueue* queue = NULL;
    int dequeueData = 0;
    printf("%c ", graph->vexs[index]);
    visited[index] = 1;
    BFS_enqueue(&queue, index);
    while (1) {
        if (BFS_dequeue(&queue, &dequeueData) == -1)
            break;
        for (int i = 0; i < graph->vexNum; ++i) {
            if ((visited[i] != 1) && graph->arcs[dequeueData][i] > 0 && graph->arcs[index][i] != MAX) {
                printf("%c ", graph->vexs[i]);
                visited[i] = 1;
                BFS_enqueue(&queue, i);
            }
        }
    }
}

/*======================================最小生成树 prim=====================================*/

typedef struct EdgePrimNode {
    char vex;
    int weight;
} EdgePrim;

// 初始化这个数组（存放当前顶点与该顶点到其他顶点的边权值）
EdgePrim* initPrimEdge(Graph* graph, int index)
{
    EdgePrim* edge = malloc(sizeof(EdgePrim) * (graph->vexNum));
    for (int i = 0; i < graph->vexNum; ++i) {
        edge[i].vex = graph->vexs[index];
        edge[i].weight = graph->arcs[index][i];
    } 
    return edge;
}

// 获取当前数组中权值最小的那个目标顶点（数组vex代表当前起始顶点，weight代表当前起始顶点到对应数组i下标终止顶点的权值）
int getMinEdge(Graph* graph, EdgePrim* edge)
{
    int min = MAX;
    int index = 0;
    for (int i = 0; i < graph->vexNum; ++i) {
        if ((edge[i].weight < min) && (edge[i].weight != 0)) {    // 0的话就代表已经遍历过该节点了
            min = edge[i].weight;
            index = i;
        }
    }
    return index;       // 这样获得最小权值数组的下标，这个下标也就是目的顶点
}

void prim(Graph* graph, int index)
{
    int min = 0;
    EdgePrim* edge = initPrimEdge(graph, index);   // 初始化好了数组（记录着该顶点以及顶点到其它顶点的最小权值）
    for (int i = 0; i < graph->vexNum - 1; ++i) {  // 第一层循环是从当前数组中获得最小权值的目标顶点
        min = getMinEdge(graph, edge);             // min得到的是当前所有最小的权值所到的目标顶点的数组下标
        printf("s:%c-->d:%c and weight = %d\n", edge[min].vex, graph->vexs[min], edge[min].weight);
        edge[min].weight = 0;                      // 打印完之后权值置0，表示该节点已经遍历过
        for (int j = 0; j < graph->vexNum; ++j) {  // 第二层循环是更新数组内容，将更小的顶点以及权值放入
            if (graph->arcs[min][j] < edge[j].weight) {
                edge[j].weight = graph->arcs[min][j];      // 更新权值
                edge[j].vex = graph->vexs[min];            // 与此同时也需要更新该权值对应的起始顶点
            }
        }
    }
}

/*======================================最小生成树 Kruskal=====================================*/

typedef struct EdgeKruskalNode {
    int start;
    int end;
    int weight;
} EdgeKruskal;

EdgeKruskal* initKruskalEdge(Graph* graph)
{
    int index = 0;
    EdgeKruskal* edge = malloc(sizeof(EdgeKruskal) * graph->arcNum);
    for (int i = 0; i < graph->vexNum; ++i) {
        for (int j = i + 1; j < graph->vexNum; ++j) {
            if (graph->arcs[i][j] != MAX) {
                edge[index].start = i;
                edge[index].end = j;
                edge[index].weight = graph->arcs[i][j];
                index++;
            }
        }
    }
    return edge;
}

// 对上面数组从小到大进行排序
void sortEdge(EdgeKruskal* edge, Graph* graph)
{
    EdgeKruskal temp;
    for (int i = 0; i < graph->arcNum - 1; ++i) {
        for (int j = 0; j < graph->arcNum - i - 1; ++j) {
            if (edge[j].weight > edge[j+1].weight) {
                temp = edge[j];
                edge[j] = edge[j+1];
                edge[j+1] = temp;
            }
        }
    }
}

void kruskal(Graph* graph)
{
    int* connected = malloc(sizeof(int) * graph->vexNum);     // 定义一个连通分量的数组，存放每个节点的连通分量，防止造成回路
    for (int i = 0; i < graph->vexNum; ++i) {   
        connected[i] = i;                                     // 每个顶点的连通分量都是i（0-5，6个顶点）                
    }
    EdgeKruskal* edge = initKruskalEdge(graph);
    sortEdge(edge, graph);
    for (int i = 0; i < graph->arcNum; ++i) {
        int startConnected = connected[edge[i].start];        // edge[i].start是数组下标
        int endConnected = connected[edge[i].end];            // 拿到对应顶点的连通分量
        if (startConnected != endConnected) {                 // 当连通分量不相等时可以连接，相等代表已经形成一条线了
            printf("s:%c-->d:%c and weight = %d\n", graph->vexs[edge[i].start], graph->vexs[edge[i].end], edge[i].weight);
            for (int j = 0; j < graph->vexNum; ++j) {         // 循环的目的是将连通分量数组内所有等于end的连通分量全都变成起始顶点的连通分量
                if (connected[j] == endConnected) {           // 在连通分量的数组内找到终点的连通分量 
                    connected[j] = startConnected;            // 终点的连通分量变成起始的连通分量
                }   
            }
        }
    }
    for (int i = 0; i < graph->vexNum; ++i)
        printf("connected[%d] = %d\n", i, connected[i]);      // 根据图推理，最后连通分量应该都是1，连到了2节点上面
}

/*======================================最短路径 dijkstra=====================================*/

int getMinEdgeWeight(int* D, int* S, Graph* graph)
{
    int index = 0;
    int min = MAX;
    for (int i = 0; i < graph->vexNum; ++i) {
        if ((S[i] != 1) && (D[i] < min)) {
            min = D[i];
            index = i;
        }
    }
    return index;   // 返回最小值节点的索引
}

void dijkstra(Graph* graph, int index, int node)    // index是我要求的起始节点的下标 (node是主函数想查看的最短节点路径)
{
    int* S = malloc(sizeof(int) * graph->vexNum);
    int* P = malloc(sizeof(int) * graph->vexNum);
    int* D = malloc(sizeof(int) * graph->vexNum);
    for (int i = 0; i < graph->vexNum; ++i) {
        if (i == index)
            S[i] = 1;               // 为1就是已经找到了该点的最小路径
        else
            S[i] = 0;
    }
    for (int i = 0; i < graph->vexNum; ++i) {
        if ((graph->arcs[index][i] > 0) && (graph->arcs[index][i] != MAX)) {
            P[i] = index;           // 此时与index传入的起始节点有边，前驱节点就是起始节点，否则为-1
        } else {
            P[i] = -1;
        }
    }
    for (int i = 0; i < graph->vexNum; ++i) {
        D[i] = graph->arcs[index][i];
    }
    // 下面这些个大的for循环是此算法核心
    // 上面初始化完三个数组，下面要开始找最小的边，加入到S数组中一位置找到最小边权值，且比较通过该节点到其他节点的距离，将小的放入数组D中
    for (int i = 0; i < graph->vexNum - 1; ++i) {
        int minIndex = getMinEdgeWeight(D, S, graph);
        S[minIndex] = 1;            // 找到最小值之后，将对应的S数组置1，代表找到了起始点到当前点的最小路径
        for (int j = 0; j < graph->vexNum; ++j) {
            if ((S[j] != 1) && (D[minIndex] + graph->arcs[minIndex][j] < D[j])) {
                D[j] = D[minIndex] + graph->arcs[minIndex][j];
                P[j] = minIndex;    // 一旦更新了最短路径，也要更新该节点的前驱，是从当前获取的最小节点过来的
            }
        }
    }

    for (int i = 0; i < graph->vexNum; ++i)
        printf("S[%d] = %d P[%d] = %d D[%d] = %d\n", i, S[i], i, P[i], i, D[i]);

    if (node == 3)                  // 找到4号节点的路径
        printf("%c->%c->%c->%c\n", graph->vexs[index], graph->vexs[P[P[node]]], graph->vexs[P[node]], graph->vexs[node]);
}

/*======================================最短路径 floyd=====================================*/

void floyd(Graph* graph)
{
    int D[graph->vexNum][graph->vexNum];
    int P[graph->vexNum][graph->vexNum];
    for (int i = 0; i < graph->vexNum; ++i) {
        for (int j = 0; j < graph->vexNum; ++j) {
            D[i][j] = graph->arcs[i][j];
            if ((graph->arcs[i][j] == MAX) || (graph->arcs[i][j] == 0)) {
                P[i][j] = -1;
            } else {
                P[i][j] = i;
            }
        }
    }
    // 算法关键部分，三层for循环
    for (int k = 0; k < graph->vexNum; ++k) {
        for (int i = 0; i < graph->vexNum; ++i) {
            for (int j = 0; j < graph->vexNum; ++j) {
                if (D[i][k] + D[k][j] < D[i][j]) {
                    D[i][j] = D[i][k] + D[k][j];
                    P[i][j] = P[k][j];
                }
            }
        }
    }
    // 打印D数组
    for (int i = 0; i < graph->vexNum; i++) {
        for (int j = 0; j < graph->vexNum; j++) {
            printf("%d ", D[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    // 打印P数组
    for (int i = 0; i < graph->vexNum; i++) {
        for (int j = 0; j < graph->vexNum; j++) {
            printf("%d ", P[i][j]);
        }
        printf("\n");
    }
}

/*======================================拓扑排序=====================================*/

typedef struct StackTopNode {
    int nodeData;
    struct StackTopNode* next;
} StackTop;

// 入栈
void pushTop(StackTop** stack, int data)
{
    StackTop* node = NULL;
    node = malloc(sizeof(StackTop));
    node->nodeData = data;
    node->next = *stack;
    *stack = node;
}

// 出栈
int popTop(StackTop** stack, int* node)
{
    if (NULL == *stack) {
        return -1;
    }
    StackTop* cur = *stack;
    *node = cur->nodeData;
    *stack = cur->next;
}

// 获取入度，每个节点被其他节点指向的个数边数
int* getInDegree(Graph* graph)
{
    int* inDegree = malloc(sizeof(int) * graph->vexNum);
    for (int i = 0; i < graph->vexNum; ++i) {
        inDegree[i] = 0;
    }
    for (int i = 0; i < graph->vexNum; ++i) {
        for (int j = 0; j < graph->vexNum; ++j) {
            if (graph->arcs[i][j] != 0 && graph->arcs[i][j] != MAX) {
                inDegree[j]++;
            }
        }
    }
    return inDegree;
}

// 用栈保存遍历到的为0的值，可以直接使用；动态规划，记忆化搜索的思想
int* topologicalSort(Graph* graph)
{
    StackTop* stack = NULL;
    int index = 0;
    int vex = 0;
    int* top = malloc(sizeof(int) * graph->vexNum);     // 这个数组保存每次输出的顶点下标
    for (int i = 0; i < graph->vexNum; ++i) {
        top[i] = 0;
    }
    int* inDegree = getInDegree(graph);                 // 获取入度
    for (int i = 0; i < graph->vexNum; ++i) {           // 将拿到的入度值中为0的全部入栈，也就是入度为0，即无前驱的节点入栈等待被取出
        if (inDegree[i] == 0) {
            pushTop(&stack, i);
        } 
    }

    while (1) {
        if (popTop(&stack, &vex) == -1) {
            break;
        }
        top[index++] = vex;                             // 将出栈的节点也就是入度为0的节点依次保存到数组中
        for (int i = 0; i < graph->vexNum; ++i) {
            if ((graph->arcs[vex][i] > 0) && (graph->arcs[vex][i] != MAX)) {  // 输出无前驱的节点后，剪掉以他为出发点的所有边
                inDegree[i]--;
                if (inDegree[i] == 0) {                 // 剪断，即减1之后的入度要是为0也入栈
                    pushTop(&stack, i);
                }
            }
        }
    }

    for (int i = 0; i < index; ++i)
        printf("%c ", graph->vexs[top[i]]);            // 输出top数组，里面也就是按照出栈顺序的没有前驱的节点，从大到小
    printf("\n");

    return top;
}

// 不用栈实现，多一层for循环，每一次都需要去遍历一下inDegree入度数组，看里面哪些为0，再将其输出记录且剪断相关的边
void topologicalSortNoStack(Graph* graph)
{
    int index = 0;
    int vex = 0;
    int* top = malloc(sizeof(int) * graph->vexNum);
    for (int i = 0; i < graph->vexNum; ++i)
        top[i] = -1;
    int* temp = malloc(sizeof(int) * graph->vexNum);        // 定义一个临时数组记录是否访问过该节点
    for (int i = 0; i < graph->vexNum; ++i)
        temp[i] = 0;                                        // 全为0代表没有遍历过该节点，为1代表节点遍历过了
    int* inDegree = getInDegree(graph);

    while (index < graph->vexNum) {
        for (int i = 0; i < graph->vexNum; ++i) {           // 每次遍历一轮找第一个为0的入度数组值
            if ((inDegree[i] == 0) && (temp[i] != 1)) {
                top[index++] = i;
                temp[i] = 1;
                for (int j = 0; j < graph->vexNum; ++j) {   // 将找到的该节点的指向都去掉
                    if ((graph->arcs[i][j] > 0) && (graph->arcs[i][j] != MAX)) {
                        inDegree[j]--;
                    }
                }
                break;              // 每次找到一次入度为0执行结束都退出重新循环找下一个为入度0的节点
            }
        } 
    }

    for (int i = 0; i < index; ++i)
        printf("%c ", graph->vexs[top[i]]);
    printf("\n");
}

/*======================================关键路径=====================================*/

int getIndex(int* top, Graph* graph, int i) {
    int j = 0;
    for(j = 0; j < graph->vexNum; j++) {
        if (top[j] == i) {
            break;
        }
    }
    return j;
}

void criticalPath(Graph* graph)
{
    int* top = topologicalSort(graph);      // 拿到拓扑排序的顶点序列
    int* early = malloc(sizeof(int) * graph->vexNum);
    int* late = malloc(sizeof(int) * graph->vexNum);
    for (int i = 0; i < graph->vexNum; ++i) {
        early[i] = 0;
        late[i] = 0;
    }
    // 计算事件节点最早发生时间
    for (int i = 0; i < graph->vexNum; ++i) {           // 遍历每一个节点，填充early数组
        int max = 0;
        for (int j = 0; j < graph->vexNum; ++j) {       // 循环去检查每个节点有多少前驱节点，获得最大的开始时间存在max中
            if (graph->arcs[j][top[i]] > 0 && graph->arcs[j][top[i]] != MAX) {      // 找top[i]的前驱节点，前驱节点看矩阵的节点那一列，有大于0且不是max的就代表有这些边指向列元素
                // 拿到拓扑排序后的节点索引，k就是为了拿到当前遍历节点V3的前驱节点V0在top拓扑排序中的下标
                // 此时j就是找到的该节点的前驱节点索引
                // 下面获取到这个前驱节点具体是在拓扑序列中的第几个元素；因为我的early数组是按照拓扑序列来存的
                int k = 0;
                for (k = 0; k < graph->vexNum; ++k) {
                    if (top[k] == j)
                        break;
                }
                if (early[k] + graph->arcs[j][top[i]] > max)    // 此时这个k就是前驱节点在early数组中的索引
                    max = early[k] + graph->arcs[j][top[i]];
            }
        }
        early[i] = max;
    }

    late[(graph->vexNum) - 1] = early[(graph->vexNum) - 1];

    // 事件节点最晚发生时间
    for (int i = (graph->vexNum) - 2; i >= 0; i--) {
        int min = MAX;
        for (int j = 0; j < graph->vexNum; j++) {
            if (graph -> arcs[top[i]][j] > 0 && graph -> arcs[top[i]][j] != MAX) {
                int index = getIndex(top, graph, j);
                if (late[index] - graph->arcs[top[i]][j] < min)
                    min = late[index] - graph->arcs[top[i]][j];
            }
        }
        late[i] = min;
    }

    // 活动指标
    for (int i = 0; i < graph->vexNum; i++) {
        for(int j = 0; j < graph->vexNum; j++) {
            if (graph->arcs[i][j] > 0 && graph->arcs[i][j] != MAX) {
                int start = getIndex(top, graph, i);
                int end = getIndex(top, graph, j);
                if ((late[end] - graph->arcs[i][j]) - early[start] == 0) {
                    printf("start = %d end = %d\n", i, j);
                }
            }
        }
    }

    for (int i = 0; i < graph->vexNum; ++i) {
        printf("%d ", early[i]);
    }
    printf("\n");
    for (int i = 0; i < graph->vexNum; ++i) {
        printf("%d ", late[i]);
    }
    printf("\n");
}

int main()
{
/*
    Node* tree = NULL;
    Node* prev = NULL;
    treeCreate(&tree, "ABD##E##CF##G##", 0, NULL);
    //treeCreate(&tree, "AB##C##", 0, NULL);       // 如果需要连续使用该函数，就需要将函数内的静态变量i变为全局变量，且归0，不然每次进函数都会递增导致段错误
    preorderTraversal(tree);
    printf("\n===============\n");
    drawTree(tree, 0);
    printf("\n===============\n");
    levelTraversal(tree);
    printf("\n===============\n");
    nonPreorderTraversal(tree);
    printf("\n===============\n");
    nonMidTraversal(tree);
    printf("\n===============\n");
    nonAfterTraversal(tree);
    printf("\n===============\n");

    //midThreadTree(tree, &prev);
    //frontThreadTree(tree, &prev);
    afterThreadTree(tree, &prev);
    //prev->right = NULL;                       // 遍历完，最后一个节点还没操作（前中序线索需要，后续最后一个节点是根不需要）
    //prev->rtag = 1;
    printf("%d", tree->left->left->rtag);
    printf("\n===============\n");
    /*
    for (Node* node = getMidFirst(tree); node != NULL; node = getMidNext(node)) {
        printf("%c ", node->data);
    }
    */
    /*
    for (Node* node = tree; node != NULL; node = getFrontNext(node))
        printf("%c ", node->data);
    */
    /*
    for (Node* node = getAfterFirst(tree); node != NULL; node = getAfterNext(node))
        printf("%c ", node->data);
    printf("\n===============\n");
    Node* BSTtree = NULL;
    char arr[] = {6, 4, 2, 2, 5, 7};
    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); ++i) {
        insertBST(&BSTtree, arr[i]);
    }
    Node* find = findBST(BSTtree, 5);
    printf("find->data = %d\n", find->data);
    drawTree(BSTtree, 0);
    printf("\n===============\n");
    printf("%d\n", getTreeDeep(BSTtree));
    
    Node* AVLtree = NULL;
    char a[] = {8,7,9,5,6};
    for (int i = 0; i < sizeof(a)/sizeof(a[0]); ++i) {
        insertAVL(&AVLtree, a[i]);
    }
    printf("\n===============\n");
    drawTree(AVLtree, 0);
    
    printf("\n===============\n");
    */

    /* // 画哈夫曼树的时候传入指针数组最后一个指针，这是根节点
    int weight[] = {5,1,3,6,11,2,4};
    Huffman* huffmanTree = initHuffmanTree(weight, sizeof(weight)/sizeof(weight[0]));
    createHuffmanTree(huffmanTree);
    for (int i = 0; i < (sizeof(weight)/sizeof(weight[0])) * 2 - 1; ++i)
        printf("%d\n", huffmanTree->digital[i]->data);
    printf("\n===============\n");
    drawTree(huffmanTree->digital[(sizeof(weight)/sizeof(weight[0])) * 2 - 2], 0);
    printf("\n===============\n");
    midTraversal(huffmanTree->digital[(sizeof(weight)/sizeof(weight[0])) * 2 - 2]);
    */

    /* // 图以及BFS和DFS遍历
    char* vex = "ABCDE";
    int arc[] = {
        0,1,1,1,0,
        1,0,1,1,1,
        1,1,0,0,0,
        1,1,0,0,1,
        0,1,0,1,0
    };
    int *visited = malloc(sizeof(int) * strlen(vex));
    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    Graph* graph = createGraph(arc, vex, strlen(vex));
    printf("DFS:\n");
    DFS(graph, visited, 0);
    printf("\n===============\n");

    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    printf("BFS:\n");
    BFS(graph, visited, 0);
    printf("\n===============\n");
    */

    /* // 最小生成树prim算法
    char* vex = "123456";
    int arc[] = {                           // 数字代表顶点之间的权值
        0, 6, 1, 5, MAX, MAX,
        6, 0, 5, MAX, 3, MAX,
        1, 5, 0, 5, 6, 4,
        5, MAX, 5, 0, MAX, 2,
        MAX, 3, 6, MAX, 0, 6,
        MAX, MAX, 4, 2, 6, 0
    };
    int *visited = malloc(sizeof(int) * strlen(vex));
    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    Graph* graph = createGraph(arc, vex, strlen(vex));
    printf("DFS:\n");
    DFS(graph, visited, 0);
    printf("\n===============\n");
    prim(graph, 0);
    */

    /* // 最小生成树kruskal算法
    char* vex = "123456";
    int arc[] = {                           // 数字代表顶点之间的权值
        0, 6, 1, 5, MAX, MAX,
        6, 0, 5, MAX, 3, MAX,
        1, 5, 0, 5, 6, 4,
        5, MAX, 5, 0, MAX, 2,
        MAX, 3, 6, MAX, 0, 6,
        MAX, MAX, 4, 2, 6, 0
    };
    int *visited = malloc(sizeof(int) * strlen(vex));
    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    Graph* graph = createGraph(arc, vex, strlen(vex));
    printf("DFS:\n");
    DFS(graph, visited, 0);
    printf("\n===============\n");
    kruskal(graph);
    printf("===============\n");
    */

    /* // 最短路径dijkstra算法
    char* vex = "1234567";
    int arc[] = {                           // 数字代表顶点之间的权值
        0, 12, MAX, MAX, MAX, 16, 14,
        12, 0, 10, MAX, MAX, 7, MAX,
        MAX, 10, 0, 3, 5, 6, MAX,
        MAX, MAX, 3, 0, 4, MAX, MAX,
        MAX, MAX, 5, 4, 0, 2, 8,
        16, 7, 6, MAX, 2, 0, 9,
        14, MAX, MAX, MAX, 8, 9, 0
    };
    int *visited = malloc(sizeof(int) * strlen(vex));
    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    Graph* graph = createGraph(arc, vex, strlen(vex));
    printf("DFS:\n");
    DFS(graph, visited, 0);
    printf("\n===============\n");
    dijkstra(graph, 0, 3);
    */

    /* // 最短路径floyd算法
    char* vex = "1234";
    int arc[] = {
        0, 1, MAX, 3,
        1, 0, 2, 2,
        MAX, 2, 0, 8,
        3, 2, 8, 0
    };
    int *visited = malloc(sizeof(int) * strlen(vex));
    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    Graph* graph = createGraph(arc, vex, strlen(vex));
    printf("DFS:\n");
    DFS(graph, visited, 0);
    printf("\n===============\n");
    floyd(graph);
    */

    /* // 拓扑排序
    char* vex = "123456";
    int arc[] = {
        0,1,1,1,0,0,
        0,0,0,0,0,0,
        0,1,0,0,1,0,
        0,0,0,0,1,0,
        0,0,0,0,0,0,
        0,0,0,1,1,0
    };
    int *visited = malloc(sizeof(int) * strlen(vex));
    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    Graph* graph = createGraph(arc, vex, strlen(vex));
    printf("DFS:\n");
    DFS(graph, visited, 0);
    printf("\n===============\n");
    topologicalSortNoStack(graph);
    */
    
    char* vex = "012345678";
    int arc[] = {
        0,   6,   4,   5,   MAX, MAX, MAX, MAX, MAX,
        MAX, 0,   MAX, MAX, 1,   MAX, MAX, MAX, MAX,
        MAX, MAX, 0,   MAX, 1,   MAX, MAX, MAX, MAX,
        MAX, MAX, MAX, 0,   MAX, 2,   MAX, MAX, MAX,
        MAX, MAX, MAX, MAX, 0,   MAX, 9,   7,   MAX,
        MAX, MAX, MAX, MAX, MAX, 0,   MAX, 4,   MAX,
        MAX, MAX, MAX, MAX, MAX, MAX, 0,   MAX, 2,
        MAX, MAX, MAX, MAX, MAX, MAX, MAX, 0,   4,
        MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, 0
    };
    int *visited = malloc(sizeof(int) * strlen(vex));
    for (int i = 0; i < strlen(vex); ++i) {
        visited[i] = 0;
    }
    Graph* graph = createGraph(arc, vex, strlen(vex));
    printf("DFS:\n");
    DFS(graph, visited, 0);
    printf("\n=================\n");
    criticalPath(graph);

    return 0;
}