#ifndef MYTBF_H__
#define MYTBF_H__

#define MYTBF_MAX   1024                    // 最多同时支持1024个不同速度且不同大小的令牌桶
typedef void mytbf_t;                       // void传给任何类型都可以

mytbf_t *mytbf_init(int cps, int burst);    // 用户传进来速率(每秒传输的字符个数)和最大令牌数，申请内存空间，返回一个令牌桶结构体的起始位置
int mytbf_fetchtoken(mytbf_t *, int);       // 取令牌，从第一个参数即这个令牌桶内取第二个参数个数的令牌；返回所取得的令牌数
int mytbf_returntoken(mytbf_t *, int);      // 还令牌，取多了没用完还给这个令牌桶；返回值为归还令牌的个数
int mytbf_destory(mytbf_t *);               // init初始化会申请内存，此处释放内存

#endif