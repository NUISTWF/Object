/* 红黑树 API  */

#include <stdio.h>
#include <stdlib.h>

typedef int KEY_TYPE;
typedef int VALUE_TYPE;

// 红黑树颜色
typedef enum {
    RED,
    BLACK
} rb_color;

// 抽出红黑树的模板
#define RBTREE_ENTRY(name, type) \
    struct name {                \
        struct type* left;       \
        struct type* right;      \
        struct type* parent;     \
        rb_color color;          \
    }

typedef struct _rbtree_node {            // 业务与实现是绑定在一起的，可迁移性很差（key和value是与红黑树的性质放在一起的）
    // 业务实现
    KEY_TYPE key;
    VALUE_TYPE value;
#if 0
    struct type* left;
    struct type* right;
    struct type* parent;
    unsigned char color;                 // 红黑树颜色（放到最后只是为了节省内存）
#endif
    // 红黑树实现 
    RBTREE_ENTRY(, _rbtree_node) node;   // 等价于 struct {...} node; node就是一个代表红黑树的结构体变量
} rbtree_node;

// struct {
//     struct _rbtree_node* left;
//     struct _rbtree_node* right;
//     struct _rbtree_node* parent;
//     unsigned char color;
// } node;

// 定义一个头指向红黑树的起始，即根节点
typedef struct _rbtree {
    rbtree_node* root;
    rbtree_node* leaf;       // 所有叶子节点都是黑色的且隐藏的（可以使他们指向同一个空节点，不能使用 NULL，因为叶子节点需要具备红黑树的性质且带颜色）
} rbtree;

/*
 * @note 红黑树的创建
 */
rbtree* rbtree_create()
{
    // 开辟一个根节点的内存空间，开辟一个叶子节点的内存空间
    rbtree* tree = malloc(sizeof(rbtree));
    rbtree_node* leaf = malloc(sizeof(rbtree_node));

    // 叶子节点固定为黑色的
    leaf->node.color = BLACK; // Leaf nodes are black
    leaf->node.left = leaf;   // Leaf points to itself
    leaf->node.right = leaf;  // Leaf points to itself
    leaf->node.parent = leaf; // Leaf points to itself
    // 初始化节点均为空的叶子节点
    tree->root = leaf;
    tree->leaf = leaf;

    return tree;
}

/*
 * @param tree 红黑树的根
 * @param rotate 待旋转分支的根节点
 * @note  左旋 LL (举例 1 2 3 依次插入，rotate相当于1，temp相当于2)
 */
void rbtree_left_rotate(rbtree* tree, rbtree_node* rotate)
{
    rbtree_node* temp = rotate->node.right;

    // 要变化三对指向
    // 1. 1和2的线断掉
    rotate->node.right = temp->node.left;
    // 变换之前需要判断2的左节点是不是leaf叶子节点
    if (temp->node.left != tree->leaf) {        // 修改2左节点的父亲
        temp->node.left->node.parent = rotate;  // 挂载2左边的子节点挂到根上去，父亲节点也要换
    }

    // 2. 修改1与父亲的连接，变成2与父亲的连接
    temp->node.parent = rotate->node.parent;    // 此时2的父亲节点需要更改
    if (rotate->node.parent == tree->leaf) {    // 如果rotate是根节点，即其父节点是空节点
        tree->root = temp;                      // 变换根节点
    } else {
        if (rotate == rotate->node.parent->node.left) {   // 如果父节点非空，说明此时挂在父节点上，但是不知道是挂载父节点的左还是右子树
            // 判断rotate是不是左孩子，如果是则2即temp就挂到左孩子上
            rotate->node.parent->node.left = temp;
        } else {
            rotate->node.parent->node.right = temp;
        }
    }

    // 3. 将1挂到2左节点上
    temp->node.left = rotate;
    rotate->node.parent = temp;
}

// 右旋 RR (左右互换即可)
void rbtree_right_rotate(rbtree* tree, rbtree_node* rotate)
{
    rbtree_node* temp = rotate->node.left;

    rotate->node.left = temp->node.right;
    if (temp->node.right != tree->leaf) {
        temp->node.right->node.parent = rotate;
    }

    temp->node.parent = rotate->node.parent;
    if (rotate->node.parent == tree->leaf) {
        tree->root = temp;
    } else {
        if (rotate == rotate->node.parent->node.right) {
            rotate->node.parent->node.right = temp;
        } else {
            rotate->node.parent->node.left = temp;
        }
    }

    temp->node.right = rotate;
    rotate->node.parent = temp;
}

// 插入后调整函数
/*
 * @param tree 红黑树的根
 * @param cur  待修改的当前数据节点
 */
void rbtree_insert_fixup(rbtree* tree, rbtree_node* cur)
{
    // 当前节点本身就是红色
    // 如果父亲是红色的就需要调整（此时cur的祖父节点肯定是黑色，因为插入之前就是标准的红黑树，叔叔节点不确定是什么节点，可能是红的可能是黑的，黑的还可能是空的叶子节点或者正常节点）
    /*
    1. 叔叔节点是红色，则叔叔父亲爷爷都要变色
    2. 叔叔节点是黑色，则需要旋转
    */

    rbtree_node* uncle = tree->leaf;

    while (cur->node.parent->node.color == RED) {   // 直到当前节点的父节点不为红则结束，意味着没有连续的两个红色节点，不需要调整红黑树了
        if (cur->node.parent->node.parent->node.left == cur->node.parent) {     // 此时cur的父亲是爷爷的左孩子
            uncle = cur->node.parent->node.parent->node.right;

            // 如果叔叔是红色节点，则叔叔父亲爷爷都要变色
            if (uncle->node.color == RED) {
                // 叔叔是红色的，意味着父亲和叔叔都是红色的，就已经平衡了，不需要旋转
                cur->node.parent->node.color = BLACK;
                cur->node.parent->node.parent->node.color = RED;
                uncle->node.color = BLACK;

                cur = cur->node.parent->node.parent;    // cur一直是红色的（此时cur指向祖父节点，因为以祖父节点为根的树已经是红黑树了，就将当前指向祖父节点向上判断），再继续循环向上判断
            } else {        // 叔叔是黑色节点
                // LR (连续执行下面两个，就相当于先LR再LL)
                if (cur == cur->node.parent->node.right) {
                    cur = cur->node.parent;
                    rbtree_left_rotate(tree, cur);
                }

                // LL
                cur->node.parent->node.color = BLACK;
                cur->node.parent->node.parent->node.color = RED;
                rbtree_right_rotate(tree, cur->node.parent->node.parent); // 传入当前节点的祖父节点和根节点进行右旋
            }
        } else {
            uncle = cur->node.parent->node.parent->node.left;

            // 如果叔叔是红色节点，则叔叔父亲爷爷都要变色
            if (uncle->node.color == RED) {
                // 叔叔是红色的，意味着父亲和叔叔都是红色的，就已经平衡了，不需要旋转
                cur->node.parent->node.color = BLACK;
                cur->node.parent->node.parent->node.color = RED;
                uncle->node.color = BLACK;

                cur = cur->node.parent->node.parent;    // cur一直是红色的（此时cur指向祖父节点，因为以祖父节点为根的树已经是红黑树了，就将当前指向祖父节点向上判断），再继续循环向上判断
            } else {        // 叔叔是黑色节点
                // RL
                if (cur == cur->node.parent->node.left) {
                    cur = cur->node.parent;
                    rbtree_right_rotate(tree, cur);
                }

                // RR
                // if (cur == cur->node.parent->node.right) {
                    cur->node.parent->node.color = BLACK;               // 父亲节点颜色变化
                    cur->node.parent->node.parent->node.color = RED;    // 祖父节点颜色变化
                    rbtree_left_rotate(tree, cur->node.parent->node.parent);
                // }
            }
        }
    }

    tree->root->node.color = BLACK; // Root is always black

    return;
}

/*
 * @param tree 红黑树的根
 * @param data 待插入的节点数据
 */
void rbtree_insert(rbtree* tree, rbtree_node* data)
{
    rbtree_node* temp = tree->root;
    rbtree_node* cur = tree->leaf;      // cur 用来找到要插入的节点地点

    // 找到要插的位置，cur的指向
    while (temp != tree->leaf) {
        cur = temp;
        // 比当前temp小，继续遍历左子树，插左子树那边
        if (data->key < temp->key) {
            temp = temp->node.left;
        } else if (data->key > temp->key) {
            temp = temp->node.right;
        } else {
            // 红黑树没有对相同的key节点有相同的定义，取决于业务场景，需要的话就修改一丢丢key的值再插入，也可以选择丢弃
            // EXIST ...
            return;
        }
    }

    // 如果一开始一个节点都没有
    if (cur == tree->leaf) {
        tree->root = data;
    } else {
        // 退出循环代表此时temp指向叶子节点，cur指向需要插入的目标判断节点
        if (data->key < cur->key) {
            cur->node.left = data;              // 节点插在左子树上
        } else if (data->key > cur->key) {
            cur->node.right = data;
        }
    }

    data->node.parent = cur;            // 填充父节点
    data->node.left = tree->leaf;       // 新插入的左右节点都是空的叶子节点，也都是黑的
    data->node.right = tree->leaf;
    // 红黑树在插入任何节点之前本身就是一个红黑树
    data->node.color = RED;             // 插入是红色，因为插入红色不一定改变此时红黑树的性质，但插入黑的肯定需要调整

    // 上色完后判断是否符合红黑树性质
    rbtree_insert_fixup(tree, data);    // 判断data此时节点的祖父节点是否满足红黑树的性质
    
    return;
}

// 打印函数
void rbtree_print_helper(rbtree_node* tree, rbtree_node* leaf)
{
    if (tree != leaf) {
        rbtree_print_helper(tree->node.left, leaf); // 递归打印（左根右）
        printf("Key: %d, Value: %d, Color: %s\n", tree->key, tree->value, tree->node.color == RED ? "RED" : "BLACK");
        rbtree_print_helper(tree->node.right, leaf);
    }
}

// 打印一个树，传入树的根
void rbtree_print(rbtree* tree)
{
    rbtree_print_helper(tree->root, tree->leaf);
}

// 释放内存
void rbtree_free_node(rbtree_node* node)
{
    if (node != NULL && node != node->node.parent) { // 避免访问无效指针
        rbtree_free_node(node->node.left);           // 递归释放左右子树
        rbtree_free_node(node->node.right);
        free(node);
    }
}

void rbtree_free(rbtree* tree)
{
    // 释放根节点
    rbtree_free_node(tree->root);
    free(tree->leaf); // 只释放叶节点
    free(tree);
}

// 传入值依次插入
void rbtree_insert_multiple(rbtree* tree, KEY_TYPE* keys, VALUE_TYPE* values, int count, int num)
{
    int cnt = num < count ? num : count;

    for (int i = 0; i < cnt; i++) {
        rbtree_node* data = malloc(sizeof(rbtree_node));
        data->key = keys[i];
        data->value = values[i];
        rbtree_insert(tree, data);
    }
}

// 查找红黑树的键值对
VALUE_TYPE rbtree_search(rbtree* tree, KEY_TYPE key)
{
    rbtree_node* current = tree->root;

    while (current != tree->leaf) {
        if (key < current->key) {
            current = current->node.left;
        } else if (key > current->key) {
            current = current->node.right;
        } else {
            return current->value; // 找到对应的值
        }
    }

    return (int)-1; // 没有找到
}

int main()
{
    rbtree* tree = rbtree_create();

    KEY_TYPE keys[] = {17, 18, 23, 34, 27, 15, 9, 6, 8, 5, 25};
    VALUE_TYPE values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    rbtree_insert_multiple(tree, keys, values, sizeof(keys) / sizeof(keys[0]), sizeof(values) / sizeof(values[0]));

    printf("Red-Black Tree Nodes:\n");
    rbtree_print(tree); // 左根右打印的

    // 查找键值
    KEY_TYPE search_key = 15;
    VALUE_TYPE value = rbtree_search(tree, search_key);
    if (value) {
        printf("Found value for key %d: %d\n", search_key, value);
    } else {
        printf("Key %d not found.\n", search_key);
    }

    // Clean up
    rbtree_free(tree);

    return 0;
}

/*
先插入17红色的，然后调整时改为黑色的，再插入18红色的，不冲突，再插入23红色的，此时冲突了，进行调整，后续一样
Red-Black Tree Nodes:
Key: 5, Value: 10, Color: RED
Key: 6, Value: 8, Color: BLACK
Key: 8, Value: 9, Color: BLACK
Key: 9, Value: 7, Color: BLACK
Key: 15, Value: 6, Color: BLACK
Key: 17, Value: 1, Color: BLACK
Key: 18, Value: 2, Color: BLACK
Key: 23, Value: 3, Color: BLACK
Key: 25, Value: 11, Color: RED
Key: 27, Value: 5, Color: RED
Key: 34, Value: 4, Color: BLACK
*/