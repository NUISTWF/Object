#ifndef INC_UNI_NLU_CONTENT_H_
#define INC_UNI_NLU_CONTENT_H_

typedef struct {
  uni_u32 key_word_hash_code; /* 存放识别词汇对应的hashcode */
  uni_u8  nlu_content_str_index; /* 存放nlu映射表中的索引，实现多个识别词汇可对应同一个nlu，暂支持256条，如果不够换u16 */
  char    *hash_collision_orginal_str /* 类似Java String equal，当hash发生碰撞时，赋值为识别词汇，否则设置为NULL */;
} uni_nlu_content_mapping_t;

enum {
  eCMD_wakeup_uni,
  eCMD_exitUni,
  eCMD_next,
  eCMD_pre,
  eCMD_zan,
  eCMD_guan,
};

const char* const g_nlu_content_str[] = {
[eCMD_wakeup_uni] = "{\"asr\":\"你好小陌\",\"cmd\":\"wakeup_uni\",\"pcm\":\"[103, 104, 105]\"}",
[eCMD_exitUni] = "{\"asr\":\"退下\",\"cmd\":\"exitUni\",\"pcm\":\"[106]\"}",
[eCMD_next] = "{\"asr\":\"下一个\",\"cmd\":\"next\",\"pcm\":\"[108]\"}",
[eCMD_pre] = "{\"asr\":\"上一个\",\"cmd\":\"pre\",\"pcm\":\"[108]\"}",
[eCMD_zan] = "{\"asr\":\"点赞\",\"cmd\":\"zan\",\"pcm\":\"[108]\"}",
[eCMD_guan] = "{\"asr\":\"关了\",\"cmd\":\"guan\",\"pcm\":\"[109]\"}",
};

/*TODO perf sort by hashcode O(logN), now version O(N)*/
const uni_nlu_content_mapping_t g_nlu_content_mapping[] = {
  {2835568846U/*你好小陌*/, eCMD_wakeup_uni, NULL},
  {954554293U/*小陌同学*/, eCMD_wakeup_uni, NULL},
  {2497873774U/*退下*/, eCMD_exitUni, NULL},
  {226981U/*滚*/, eCMD_exitUni, NULL},
  {4282169889U/*下一个*/, eCMD_next, NULL},
  {2440615472U/*往下*/, eCMD_next, NULL},
  {1775111442U/*不好看*/, eCMD_next, NULL},
  {1155227591U/*没意思*/, eCMD_next, NULL},
  {3394666208U/*上一个*/, eCMD_pre, NULL},
  {2440615471U/*往上*/, eCMD_pre, NULL},
  {670783532U/*刚刚那个*/, eCMD_pre, NULL},
  {856029107U/*刷快了*/, eCMD_pre, NULL},
  {2444164371U/*点赞*/, eCMD_zan, NULL},
  {228721U/*赞*/, eCMD_zan, NULL},
  {3441288465U/*疯狂点赞*/, eCMD_zan, NULL},
  {2389494173U/*关了*/, eCMD_guan, NULL},
  {2389497932U/*关闭*/, eCMD_guan, NULL},
  {2389494734U/*关掉*/, eCMD_guan, NULL},
};

#endif
