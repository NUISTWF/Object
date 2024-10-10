/* B树 API */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef int KEY_TYPE;
typedef char VALUE_TYPE;

#define M 6             // B树的阶数，6阶树表示每个节点最多有6个孩子节点
#define SUB_M (M) / 2   // 子树的最小分支数

/* B树的节点结构 */
typedef struct _btree_node {
    KEY_TYPE* keys;                 // 关键字数组
    VALUE_TYPE* values;             // 值数组
    struct _btree_node** children;  // 孩子节点指针数组
    int num;                        // 当前节点的关键字个数
    bool leaf;                      // 是否是叶子节点
} btree_node;

/* B树结构 */
typedef struct _btree {
    struct _btree_node* root;        // 根节点
} btree;

/* 创建一个新的B树节点 */
static btree_node* create_btree_node(bool leaf)
{
    btree_node* node = (btree_node*)malloc(sizeof(btree_node));
    node->keys = (KEY_TYPE*)malloc((M - 1) * sizeof(KEY_TYPE));   // 分配 M-1 个关键字
    node->values = (VALUE_TYPE*)malloc((M - 1) * sizeof(VALUE_TYPE)); // 分配 M-1 个值
    node->children = (btree_node**)malloc(M * sizeof(btree_node*));  // 分配 M 个孩子指针
    node->num = 0;      // 当前节点的关键字数目初始化为 0
    node->leaf = leaf;  // 设置节点是否是叶节点
    return node;
}

/* 创建B树 */
btree* create_btree()
{
    btree* tree = (btree*)malloc(sizeof(btree));
    tree->root = create_btree_node(true);  // 初始时根节点为空并且是叶子节点
    return tree;
}

/* 分裂节点 */
static void split_child(btree_node* parent, int i)
{
    btree_node* child = parent->children[i];
    btree_node* new_node = create_btree_node(child->leaf);
    new_node->num = SUB_M - 1;

    // 将 child 中后半部分的 keys 和 values 复制到 new_node
    for (int j = 0; j < SUB_M - 1; j++) {
        new_node->keys[j] = child->keys[j + SUB_M];
        new_node->values[j] = child->values[j + SUB_M];
    }

    // 如果 child 不是叶子节点，将 child 的子节点也拆分
    if (!child->leaf) {
        for (int j = 0; j < SUB_M; j++) {
            new_node->children[j] = child->children[j + SUB_M];
        }
    }

    child->num = SUB_M - 1;  // 更新 child 中的关键字数量

    // 将新节点插入到 parent 的 children 数组中
    for (int j = parent->num; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = new_node;

    // 将 child 的中间关键字提升到 parent 中
    for (int j = parent->num - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->values[j + 1] = parent->values[j];
    }
    parent->keys[i] = child->keys[SUB_M - 1];
    parent->values[i] = child->values[SUB_M - 1];
    parent->num++;
}

/* 插入非满节点 */
static void insert_non_full(btree_node* node, KEY_TYPE key, VALUE_TYPE value)
{
    int i = node->num - 1;

    if (node->leaf) {
        // 叶子节点，直接插入
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->num++;
    } else {
        // 非叶子节点，找到插入位置
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;

        // 如果子节点已满，先分裂
        if (node->children[i]->num == M - 1) {
            split_child(node, i);
            if (key > node->keys[i]) {
                i++;
            }
        }
        insert_non_full(node->children[i], key, value);
    }
}

/* 插入操作 */
void insert_btree(btree* tree, KEY_TYPE key, VALUE_TYPE value)
{
    btree_node* root = tree->root;

    if (root->num == M - 1) {
        // 根节点已满，需要分裂
        btree_node* new_root = create_btree_node(false);
        new_root->children[0] = root;
        split_child(new_root, 0);
        tree->root = new_root;

        // 插入到合适的子节点
        insert_non_full(new_root, key, value);
    } else {
        insert_non_full(root, key, value);
    }
}

/* 打印B树 */
void print_btree(btree_node* node, int level)
{
    if (node != NULL) {
        for (int i = 0; i < node->num; i++) {
            if (!node->leaf) {
                print_btree(node->children[i], level + 1);
            }
            printf("Level %d: Key = %d; Value = %c\n", level, node->keys[i], node->values[i]);
        }
        if (!node->leaf) {
            print_btree(node->children[node->num], level + 1);
        }
    }
}

/* 销毁B树节点 */
static void destroy_btree_node(btree_node* node)
{
    if (node != NULL) {
        if (!node->leaf) {
            for (int i = 0; i <= node->num; i++) {
                destroy_btree_node(node->children[i]);
            }
        }
        free(node->keys);
        free(node->values);
        free(node->children);
        free(node);
    }
}

/* 销毁B树 */
void destroy_btree(btree* tree)
{
    if (tree != NULL) {
        destroy_btree_node(tree->root);
        free(tree);
    }
}

int main()
{
    // 创建B树
    btree* tree = create_btree();

    // 插入元素
    for (int i = 0; i < 26; i++) {
        insert_btree(tree, i + 1, 'A' + i);
    }

    // 打印B树
    printf("B树结构:\n");
    print_btree(tree->root, 0);

    // 销毁B树
    destroy_btree(tree);

    return 0;
}

/*
B树结构:
Level 2: Key = 1; Value = A
Level 2: Key = 2; Value = B
Level 1: Key = 3; Value = C
Level 2: Key = 4; Value = D
Level 2: Key = 5; Value = E
Level 1: Key = 6; Value = F
Level 2: Key = 7; Value = G
Level 2: Key = 8; Value = H
Level 0: Key = 9; Value = I
Level 2: Key = 10; Value = J
Level 2: Key = 11; Value = K
Level 1: Key = 12; Value = L
Level 2: Key = 13; Value = M
Level 2: Key = 14; Value = N
Level 1: Key = 15; Value = O
Level 2: Key = 16; Value = P
Level 2: Key = 17; Value = Q
Level 1: Key = 18; Value = R
Level 2: Key = 19; Value = S
Level 2: Key = 20; Value = T
Level 1: Key = 21; Value = U
Level 2: Key = 22; Value = V
Level 2: Key = 23; Value = W
Level 2: Key = 24; Value = X
Level 2: Key = 25; Value = Y
Level 2: Key = 26; Value = Z
*/