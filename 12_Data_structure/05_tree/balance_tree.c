/* 二叉树 
 * 实现了：增，删，改，查，平衡，前中后序遍历
 * 节点数，深度，画树，查找父亲节点，删叶子节点，删根节点，删整个树
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    int data;
	struct TreeNode *left;
    struct TreeNode *right;
} Node;

// （增加）树的插入，小的放左，大的放右
void insert(Node** tree, int data)
{
    Node* node = NULL;
    if (*tree == NULL) {
        node = malloc(sizeof(Node));
        node->data = data;
        node->left = NULL;
        node->right = NULL;
        *tree = node;
        return ;
    }

    if (data < (*tree)->data) {
        insert(&((*tree)->left), data);
    } else {
        insert(&((*tree)->right), data);
    }
}

// 节点数 = 左节点数 + 右节点数 + 1
int getNodes(Node* tree)
{
    if (tree == NULL)
        return 0;
    return (getNodes(tree->left) + getNodes(tree->right) + 1);
}

// 树的深度 = MAX(左子树的深度, 右子树的深度) + 1
int treeDeep(Node *tree)
{
    if (tree == NULL)
        return 0;
    return (treeDeep(tree->left) > treeDeep(tree->right) ? treeDeep(tree->left) : treeDeep(tree->right)) + 1;
}

// 按层遍历（利用队列实现，根节点先放入，出队的时候后面放出该节点的左右孩子）
void layerTraverse(Node* tree)
{   

}

// 前序遍历 根左右
void preorderTraversal(Node* tree)
{
    if (NULL == tree)
        return ;
    printf("%d ", tree->data);          // 微操作
    preorderTraversal(tree->left);      // 超级操作
    preorderTraversal(tree->right);
}

// 中序遍历 左根右
void midorderTraversal(Node* tree)
{
    if (NULL == tree)
        return ;
    midorderTraversal(tree->left);      // 超级操作
    printf("%d ", tree->data);          // 微操作
    midorderTraversal(tree->right);
}

// 后序遍历 左右根
void afterwordTraversal(Node* tree)
{
    if (NULL == tree)
        return ;
    afterwordTraversal(tree->left);     // 超级操作
    afterwordTraversal(tree->right);
    printf("%d ", tree->data);          // 微操作
}

// 画树就是右根左
void drawTree(Node* tree, int level)
{
    if (NULL == tree)
        return ;
    drawTree(tree->right, level+1);
    for (int i = 0; i < level; ++i) {
        printf("    ");
    }
    printf("%d", tree->data);
    printf("\n");
    drawTree(tree->left, level+1);
}

static Node* findRight(Node* tree)
{
    if (NULL == tree->right)
        return tree;
    findRight(tree->right);
}

static void turnRight(Node** tree)
{
    Node* root = *tree;
    *tree = root->left;
    root->left = NULL;
    findRight(*tree)->right = root;
}

// 找到最左边下边的节点
static Node* findLeft(Node* tree)
{
    if (NULL == tree->left)         // 不断的循环去找左节点，直至叶子节点为空的时候返回该节点的位置
        return tree;
    findLeft(tree->left);
}

// 左旋有三步：首先左旋是右孩子作为根节点，然后将本来的根节点右孩子置空，最后将这个根节点挂到新的根节点的最左边去
static void turnLeft(Node** tree)   // 传二级指针是为了改变main中tree的指向，通过指针的地址再取*去改变tree的指向
{                                   // 如果直接传入一级指针，只能修改指向的地址上的数据，改变不了tree的指向；如果是一级指针，顶多就是让形参tree指向其他地方，没有改变本来的tree指向
    Node* root = *tree;             // 保存本来的根节点
    *tree = root->right;            // 本来根节点的右孩子变为新的根节点
    root->right = NULL;             // 本来根节点是有右孩子的，需要置空
    findLeft(*tree)->left = root;   // 找到此时新的根节点的最左边的叶子节点，将本来的根节点连过去
}

// 平衡二叉树，相对根节点左右子节点个数差值为小于等于1
void balanceTree(Node** tree)
{
    if (NULL == *tree)
        return ;
    while (1) {                     // 不断的左旋右旋，直至差值为-1 0 1，意味着本次根节点平衡结束
        int sub = getNodes((*tree)->left) - getNodes((*tree)->right);
        if (sub >= -1 && sub <= 1)
            break;
        if (sub < -1) {
            turnLeft(tree);
        } else {
            turnRight(tree);
        }
    }
    // 当前根节点平衡完，就去平衡左右子树；因为balance函数就是平衡当前根节点的
    balanceTree(&((*tree)->left));
    balanceTree(&((*tree)->right));
}

// （查找）根据数据查找节点
Node* findByData(Node* tree, int data)
{
    if (NULL == tree)
        return NULL;
    if (tree->data == data)
        return tree;
    if (data < tree->data)
        return findByData(tree->left, data);
    else
        return findByData(tree->right, data);
}

// 查找当前需要查找节点的父节点
Node* findFatherByData(Node* tree, int data)
{
    if ((tree->left) != NULL) {
        if ((tree->left)->data == data)
            return tree;
    }
    if ((tree->right) != NULL) {
        if ((tree->right)->data == data)
            return tree;
    }

    if (data < tree->data) {
        if (tree->left != NULL)
            return findFatherByData(tree->left, data);
        else
            return NULL;
    } else {
        if (tree->right != NULL)
            return findFatherByData(tree->right, data);
        else
            return NULL;
    }
}

// （修改数据）
void changeData(Node** tree, int olddata, int newdata)
{
    if (NULL == *tree)
        return ;
    if ((*tree)->data == olddata) {
        (*tree)->data = newdata;
        return ;
    }
    if (olddata < (*tree)->data)
        changeData(&((*tree)->left), olddata, newdata);
    else
        changeData(&((*tree)->right), olddata, newdata);
}

// 查找到对应的节点看是不是叶子节点（左右都是空即是叶子节点）
Node* IsLeafNode(Node* tree, int data)
{
    Node* findNode = findByData(tree, data);
    if (NULL == findNode) {
        printf("No data = %d node!\n", data);
        return NULL;
    } else {
        if ((findNode->left == NULL) && (findNode->right == NULL))
            return findNode;
        else
            return NULL;
    }
}

// 删除某个节点（如果是叶子节点则直接删，如果是根节点则让左边子节点顶上来作为根节点）
void deleteNode(Node** tree, int data)
{
    Node* root = *tree;                                    // 保存原来的根节点
    Node* node = findByData(*tree, data);                  // 找到要删除的节点
    Node* fatherNode = findFatherByData(*tree, data);      // 找到要找节点的父节点
    if (IsLeafNode(*tree, data)) {                         // 如果是叶子节点就直接删
        if (fatherNode->left->data == data) {              // 判断该要删除的节点是找到父亲的左节点还是右节点
            free(fatherNode->left);
            fatherNode->left = NULL;
        } else {
            free(fatherNode->right);
            fatherNode->right = NULL;
        }
    } else {
        // 下面是删除根节点的情况
        *tree = node->left;                                // 删除根节点就将左边节点顶上作为新根
        Node* findRightNode = findRight(*tree);            // 找到当前新根最右的节点
        findRightNode->right = root->right;                // 将原来根的右子树挂载到这个新根的最右边
        free(node);
    }
}

// 整个树全删掉（递归删除）
void deleteTree(Node* tree)                                // 释放树的时候，最后释放当前节点，先将左右子树释放
{
    if (tree == NULL)
        return ;
    deleteTree(tree->left);                                // 递归删除左子树和右子树
    deleteTree(tree->right);
    free(tree);                                            // 释放当前节点的内存
}

int main()
{
    Node* tree = NULL;
    int arr[9] = {1, 2, 3, 7, 6, 5, 9, 8, 4};
    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); ++i) {
        insert(&tree, arr[i]);
    }
    printf("tree depth = %d\n", treeDeep(tree));
    int number = getNodes(tree);
    printf("tree nodes = %d\n", getNodes(tree));
    preorderTraversal(tree);
    printf("\n");
    drawTree(tree, 0);
    balanceTree(&tree);
    printf("=============================\n");
    drawTree(tree, 0);
    Node* findNode = findByData(tree, 8);
    printf("find node data = %d\n", findNode->data);
    changeData(&tree, 1, 0);
    printf("=============================\n");
    drawTree(tree, 0);
    deleteNode(&tree, 5);
    printf("=============================\n");
    drawTree(tree, 0);
    deleteNode(&tree, 8);
    printf("=============================\n");
    drawTree(tree, 0);
    midorderTraversal(tree);
    printf("\n");
    deleteTree(tree);
    printf("=============================\n");
    drawTree(tree, 0);
    return 0;
}
