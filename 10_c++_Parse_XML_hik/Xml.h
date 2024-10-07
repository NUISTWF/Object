#pragma once

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <sstream>
#include <fstream>

namespace hik_wangfei105 {

namespace xml {
using namespace std;        // 在我的 xml 命名空间中，使用 std 标准命名空间
#define XML_LOG     Logger().getSS()

/* 日志类 */
class Logger {
public:
	Logger();
	~Logger();
	stringstream& getSS();  // 返回引用类型，这样外部就是直接操作的类中变量 m_ss （因为要在外部获取打印的内容到 m_ss 中）

private:
	stringstream m_ss;
};

/* XML 设计类 */
class Xml {
public:
    /* 构造/析构（支持多种参数的构造，char*, string, 拷贝构造） */
    Xml();
    Xml(const char* name);
    Xml(const string& name);
    Xml(const Xml& other);  // 如果不提供，则编译器会自动提供浅拷贝
    ~Xml();

    /* 获取和设置名字 */
    string name() const;
    void name(const string& name);
    
    /* 获取和设置文本 */
    string text() const;
    void text(const string& text);

    /* 获取和设置属性 */
    string attr(const string& key) const;
    void attr(const string& key, const string& value);

    /* 重载中括号运算符（主要是操作子节点），以数组形式或者节点名字设置/获取第几个孩子节点 */
    Xml& operator [] (const int index);
    Xml& operator [] (const char* name);
    Xml& operator [] (const string& name);

    /* 增加孩子节点 */
    void append(const Xml& child);

    /* 子节点个数 */
    int size();

    /* 释放内存 */
    void clear();
    
    /* 删除子节点（支持索引、char*名字、string名字删除） */
    void remove(int index);
    void remove(const char* name);
    void remove(const string& name);

    /* 重载赋值的构造函数 */
    Xml& operator = (const Xml& other);

    /* 序列化（将 c++ 对象转为 xml 字符串） */
    string serialization();

    /* 定义 Xml 类的迭代器   需要遍历某节点下面的子节点 */
    typedef list<Xml>::iterator iterator;  // 定义下 xml 的迭代器类型，外部直接使用 iterator
    iterator begin()
    {
        return m_child.begin();            // 起始 begin 就是第一个子节点，迭代器指向这些子节点 
    }

    iterator end()
    {
        return m_child.end();
    }

    iterator erase(iterator it)      /* 删除迭代器指向的节点，也就是删除子节点 */
    {
        it->clear();
        return m_child.erase(it);    /* erase 返回的是下一个迭代器 */
    }

    /* 导入 XML 文件来解析 */
    bool load(const string& filename);

    /* 将采集到的数据保存到文件内 */
    bool save(const string& filename);

    /* 导入字符串来解析 XML */
    bool parse(const string& str);

private:                            /* 也可以更新为指针形式，更能节省空间（但要注意深浅拷贝和释放内存） */
    string* m_name;                 /* 节点名字 */
    string* m_text;                 /* 节点文本 */
    map<string, string> m_attrs;    /* 节点属性 */
    list<Xml> m_child;              /* 孩子节点 */
};

/* XML 解析类 */
class Parser {
public:
    /* 构造/析构 */
    Parser();
    ~Parser();

    /* 导入文件或者字符串加载 XML */
    bool load_file(const char* filename);
    bool load_file(const string& filename);
    bool load_string(const char* str);
    bool load_string(const string& str);

    /* 支持直接传入文件名字或者字符串解析返回一个 XML 对象 */
    Xml parse_file(const string& filename);
    Xml parse_str(const string& str);
    Xml parse_file(const char* filename);
    Xml parse_str(const char* str);

    /* 解析 XML 字符串返回一个 Xml 对象 */
    Xml parse();

private:
    /*  跳过空白字符 */
    void skip_white_space();

    /* 解析 XML 声明 */
    bool parse_declaration();

    /* 解析 XML 注释 */
    bool parse_comment();

    /* 解析元素正文（即除了注释和声明的） */
    Xml parse_element();

    /* 解析元素名字 */
    string parse_element_name();

    /* 解析元素文本 */
    string parse_element_text();

    /* 解析元素属性键 */
    string parse_element_attr_key();

    /* 解析元素属性值 */
    string parse_element_attr_val();
    
private:
    string m_str;   /* 待解析的字符串 */
    int m_idx;      /* 需要解析字符串的字符下标 */
};
}

}