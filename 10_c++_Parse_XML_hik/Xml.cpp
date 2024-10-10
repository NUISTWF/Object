#include "Xml.h"

using namespace hik_wangfei105::xml;

/* 日志空构造 */
Logger::Logger()
{

}

/* 日志在析构时打印信息（m_ss 是从外部获取到的流字符串） */
Logger::~Logger()
{
	cout << m_ss.str() << endl;
}

stringstream& Logger::getSS()
{
    return m_ss;
}

/* 无参构造 */
Xml::Xml() : m_name(NULL), m_text(NULL), m_attrs(), m_child()
{

}

/* 初始化名字 */
Xml::Xml(const char* name) : m_name(NULL), m_text(NULL), m_attrs(), m_child()
{
    m_name = new string(name);  /* 指针需要开辟内存 */
}

Xml::Xml(const string& name) : m_name(NULL), m_text(NULL), m_attrs(), m_child()
{
    m_name = new string(name);
}

Xml::Xml(const Xml& other)  /* 如果参数定义为指针要注意深拷贝代替浅拷 */
{
    m_name = new string(*(other.m_name));
    m_text = new string(*(other.m_text));

    // m_attrs = new map<string, string>();    /* 开辟空间，一一赋值 */
    // for (const auto& pair : other.m_attrs)
    // {
    //     (*m_attrs)[pair.first] = pair.second;
    // }

    // m_child = new list<Xml>();
    // for (const auto& element : other.m_child)
    // {
    //     m_child->push_back(element);  // 将每个 Xml 对象拷贝到 m_child 中（有问题，每个 Xml 对象又都是指针分配了空间）
    // }

    /* 方便起见，直接简单的值拷贝（浅拷贝） */
    m_attrs = other.m_attrs;
    m_child = other.m_child;
}

/* 空析构 */
Xml::~Xml()
{
    /* 如果定义的是指针形式需要释放内存 */
    if (m_name != NULL)
    {
        delete m_name;
        m_name = NULL;
    }

    if (m_text != NULL)
    {
        delete m_text;
        m_text = NULL;
    }
}

/* 获取名字 */
string Xml::name() const        /* 成员函数加const代表函数内不会修改任何成员变量 */
{
    if (NULL == m_name)
    {
        return "";
    }
    return *m_name;             /* m_name 是个指针，返回解引用的值（返回名字） */
}

/* 设置名字 */
void Xml::name(const string& name)
{
    if (m_name != NULL)         /* 非空就释放再重新开辟赋值，也可以直接覆盖本来空间的值 */
    {
        delete m_name;
        m_name = NULL;
    }
    m_name = new string(name);
}

// 也可以不重新分配内存，直接赋值原有的内存，如果是空的话再夫开辟新空间
// void Xml::name(const string& name)
// {
//     if (m_name != NULL)
//     {
//         *m_name = name;  // 直接覆盖 m_name 指向的 string 的内容
//     }
//     else
//     {
//         m_name = new string(name);  // 如果 m_name 是空指针，分配新内存
//     }
// }

/* 获取文本 */
string Xml::text() const
{
    if (NULL == m_text)
    {
        return "";
    }

    return *m_text;
}

/* 设置文本 */
void Xml::text(const string& text)
{
    if (m_text != NULL)
    {
        delete m_text;
        m_text = NULL;
    }

    m_text = new string(text);
}

/* 获取属性（提供键，获取值 ）*/
string Xml::attr(const string& key) const
{
    map<string, string>::const_iterator it = m_attrs.find(key);
    if (it != m_attrs.end())
    {
        return it->second;  /* 返回找到的键值对的值 */
    }

    XML_LOG << "attr is not found.";    /* 没找到属性打印信息 */
    return "";
}

/* 设置属性（设置键值对，map存储键值对） */
void Xml::attr(const string& key, const string& value)
{
    // if (NULL == m_attrs)
    // {
    //     m_attrs = new map<string, string>();
    // }

    /* 非指针直接赋值即可 */
    m_attrs[key] = value;
}

/* 重载 [] 运算符，方便直接对象加[]进行修改以及获取（根据索引来找第几个子节点） */
Xml& Xml::operator [] (int index)  /* 返回 Xml 是返回修改后的值 */
{
    int size = 0;

    if (index < 0)
    {
        XML_LOG << "index less than zero.";
    }

    // if (NULL == m_child)
    // {
    //     m_child = new list<Xml>();
    // }

    size = m_child.size();              // 获取到多少个子节点
    // XML_LOG << size;
    if (index >= 0 && index < size)     // 如果 index 是范围内的就返回对应的子节点，被设置为外部等号的右值（返回引用类型被设置）
    {
        Xml::iterator it = m_child.begin();

        for (int i = 0; i < index; i++)
        {
            it++;
        }

        return *it; // 返回想要获取的第几个子节点（放在等号右边就是返回对应数值）
    }
    XML_LOG << "121";
    m_child.push_back(Xml());
    XML_LOG << "113";
    if (index >= size)
    {
        for (int i = size; i <= index; i++)     /* 扩容，扩到我指定的 index 处 */
        {
            XML_LOG << "111";
            m_child.push_back(Xml());           /* 全部插入空对象 */
            XML_LOG << "222";
        }
    }
    // 为啥要扩容且返回我要的 index 位置的内容？因为可能是外部被设置一个新的子节点在该位置

    return m_child.back();                      /* 返回最后的，即 index 需要获取的 */
}

/* 支持传入 char* 去转为 string 不再重复实现 */
Xml& Xml::operator [] (const char* name)
{
    return (*this)[string(name)];               /* (*this) 是外部的 XML 对象，调用 [] 运算符 */
}

/* 实现按照孩子名字来找子节点（返回找到的第一个该名字的子节点） */
Xml& Xml::operator [] (const string& name)
{
    // if (NULL == m_child)
    // {
    //     m_child = new list<Xml>();
    // }

    for (Xml::iterator it = m_child.begin(); it != m_child.end(); it++)
    {
        if (it->name() == name)     // it 是指向list中的xml类型的子节点
        {
            return *it;
        }
    }

    // XML_LOG << "1";
    /* 没找到此名字的节点就原地保存该节点到孩子列表中 */
    m_child.push_back(Xml(name));

    return m_child.back();     /* 再返回最后一个子节点 */
}

/* 添加子节点 */
void Xml::append(const Xml& child)
{
    // if (NULL == m_child)
    // {
    //     m_child = new list<Xml>();
    // }

    if (!child.name().empty())
    {
        m_child.push_back(child);
    }
    else
    {
        XML_LOG << "add child is NULL";
    }
}

/* 获取子节点的个数 */
int Xml::size()
{
    return m_child.size();
}

/* 释放节点资源 */
void Xml::clear()
{
    if (m_name != NULL)
    {
        delete m_name;
        m_name = NULL;
    }

    if (m_text != NULL)
    {
        delete m_name;
        m_name = NULL;
    }

    // if (m_attrs != NULL)
    // {
    //     for (map<string, string>::iterator it = m_attrs->begin(); it != m_attrs->end(); it++)
    //     {
    //         m_attrs->erase(it);
    //     }
    //     delete m_attrs;
    //     m_attrs = NULL;
    // }

    // if (m_child != NULL)
    // {
    //     /* 注意孩子节点里面的一个 list 的每一项存储的 Xml 对象都需要释放 */
    //     for (Xml::iterator it = m_child->begin(); it != m_child->end(); it++)
    //     {
    //         (*it).clear();
    //     }
    //     /* 全释放完成就释放本节点 */
    //     delete m_child;
    //     m_child = NULL;
    // }

    return ;
}

/* 删除子节点（索引方式） */
void Xml::remove(int index)
{
    int size = 0;

    // if (NULL == m_child)
    // {
    //     return ;
    // }

    size = m_child.size();

    if (index < 0 || index >= size)
    {
        XML_LOG << "index does not exist in the value range.";
        return ;
    }

    Xml::iterator it = m_child.begin();
    for (int i = 0; i < index; i++)
    {
        it++;
    }
    // 找到我要删除的节点迭代器 it

    (*it).clear();      /* 找到对应的 list 中存放的 Xml 对象，释放内存 */
    m_child.erase(it);  /* 删除 list 中的该节点 */

    return ;
}

/* 直接调用stirng实现函数即可 */
void Xml::remove(const char* name)
{
    remove(string(name));
}

/* 删除子节点（节点名方式） */
void Xml::remove(const string& name)
{
    // if (NULL == m_child)
    // {
    //     return;
    // }

    /* 子节点有同样名称，边遍历边删除，直到删除所有同名的（找是找一个，删除是删除所有的） */
    for (Xml::iterator it = m_child.begin(); it != m_child.end(); )
    {
        if (it->name() == name)
        {
            it->clear();
            it = m_child.erase(it);   /* 删除后返回的是指向下一个迭代器 */
        }
        else
        {
            it++;
        }
    }

    /* 如果没找到该名字的节点则返回空 */
    return ;
}

Xml& Xml::operator = (const Xml& other)
{
    clear();    /* 先 clear 清空，再深浅拷贝 */

    m_name = new string(*(other.m_name));   // 此处的 bug 巨他妈的恶心，找半天（重载 Xml 类型的 = 运算符的时候没有给目标变量开辟空间）
    m_text = new string(*(other.m_text));

    /* 这里还有问题（map和list不能这么直接赋值，需要一个一个赋值） */
    m_attrs = other.m_attrs;
    m_child = other.m_child;

    return *this;          // 返回自身可以进行链式编程，较少资源消耗等
}

/* 序列化（将 xml 对象转为一长串的字符串，注：针对一个最简的xml序列化，内部使用递归序列） */
string Xml::serialization()
{
    stringstream ss;

    if (NULL == m_name)
    {
        XML_LOG << "error! name is NULL!" << __LINE__;
    }

    /* 开始序列化（将xml对象的内容按照xml的格式转为字符串） */
    ss << "<" << *m_name;   // <students
    if (!m_attrs.empty())   // 判断有无属性（有属性就去序列化）
    {
        for (map<string, string>::iterator it = m_attrs.begin(); it != m_attrs.end(); it++) // 循环添加属性
        {
            ss << " " << it->first << "=" << "\"" << it->second << "\"";
        }
    }
    ss << ">";              // 此时序列化：<student id="1" class="101">
    /* 名字头序列化完成 */

    /* 如果有子节点就开始序列化子节点（文本内容在最内层，应该在此部分的下面） */
    if (!m_child.empty())
    {
        for (Xml::iterator it = m_child.begin(); it != m_child.end(); it++)
        {
            ss << it->serialization();     /* 子节点的序列化（每个子节点也都是一个xml对象，递归调用即可） */
        }
    }

    /* 直到没有子节点，也就是到最里面了，才开始找文本内容 <name>Jack</name> */
    if (m_text != NULL)
    {
        ss << *m_text;
    }
    
    /* 结束标签 */
    ss << "</" << *m_name << ">";

    return ss.str();
}

/* xml对象直接导入文件进行解析 */
bool Xml::load(const string& filename)
{
    Parser p;
    
    if (!p.load_file(filename))   // 导入文件
    {
        return false;
    }

    *this = p.parse();            // 解析文件内的xml

    return true;
}

/* 将解析后的xml保存到文件中 */
bool Xml::save(const string& filename)
{
    ofstream fout(filename.c_str());  // 输出文件流，用于向文件写入数据

    if (fout.fail())
    {
        return false;
    }

    fout << serialization();          // 将xml对象序列化后保存到文件内（此时调用save的xml对象已经被完全赋值了）
    fout.close();

    return true;
}

/* 导入字符串进行解析，然后填充到xml对象 */
bool Xml::parse(const string& str)
{
    Parser p;

    if (!p.load_string(str))
    {
        return false;
    }

    *this = p.parse();

    return true;
}

/* 解析类的构造函数 */
Parser::Parser() : m_str(""), m_idx(0)
{

}

/* 解析类的析构函数 */
Parser::~Parser()
{

}

/* 支持导入xml文件进行解析 */
bool Parser::load_file(const string& filename)
{
    stringstream ss;                // ifstream 是输入文件流，从文件读取数据
    
    ifstream fin(filename.c_str()); // （构造函数直接打开） 转为 char* 类型，匹配 ifstream fin 对象的构造函数（本编译不支持直接 filename）
    if (fin.fail())
    {
        return false;
    }

    ss << fin.rdbuf();
    
    m_str = ss.str();
    m_idx = 0;

    // XML_LOG << m_str;    /* 测试语句 */
    fin.close();

    return true;
}

/* 导入文件char*的名字 */
bool Parser::load_file(const char* filename)
{
    return load_file(string(filename));
}

/* 直接导入string类型字符串进行解析 */
bool Parser::load_string(const string& str)
{
    m_str = str;
    m_idx = 0;

    return true;
}

/* 直接导入char*类型字符串进行解析 */
bool Parser::load_string(const char* str)
{
    return load_string(string(str)); 
}

/* 跳过空格等字符 */
void Parser::skip_white_space()
{   /* 当遇到空格等字符时，继续往下读一个字符（移动 m_idx） */
    while (m_str[m_idx] == ' ' || m_str[m_idx] == '\r' || m_str[m_idx] == '\n' || m_str[m_idx] == '\t')
    {                   // while 是防止多个空格或者特殊符号连一起，全都忽略
        m_idx++;        // 遇到空格或换行或制表符就忽略
    }
}

/* 直接解析xml文件（不需要先导入再解析，直接一步到位也行），支持 string 类型文件名 */
Xml Parser::parse_file(const string& filename)
{
    load_file(filename);
    return parse();
}

/* 支持直接导入字符串解析 */
Xml Parser::parse_str(const string& str)
{
    load_string(str);
    return parse();
}

/* 支持 char* 类型文件名 */
Xml Parser::parse_file(const char* filename)
{
    load_file(filename);
    return parse();
}

/* 支持 char* 类型字符串 */
Xml Parser::parse_str(const char* str)
{
    load_string(str);
    return parse();
}

/* 解析 XML 的声明（就是将 m_idx 跳过声明） */
bool Parser::parse_declaration()
{
    unsigned int pos = 0;

    if (m_str.compare(m_idx, 5, "<?xml") != 0)  /* 解析声明时验证下是不是声明的字符串 */
    {
        return false;
    }

    m_idx += 5;
    pos = m_str.find("?>", m_idx);       // 查找 "?>" 的位置，因为解析 declaration 需要找到 "?>" 结束标记
    if (string::npos == pos)             // 如果相等代表没有找到（string:: 代表 string 类中的无效位置）
    {
        return false;
    }

    m_idx = pos + 2;   // 此时 idx 指向换行符，即指向声明的最后一个字符（声明结束了）
    return true;
}

/* 解析注释（说白了是将 m_idx 跳过该注释） */
bool Parser::parse_comment()
{
    unsigned int pos = 0;

    if (m_str.compare(m_idx, 4, "<!--") != 0)       /* 解析注释时验证下是不是注释起始字符 */
    {
        return false;
    }

    m_idx += 4;
    pos = m_str.find("-->", m_idx);       // 查找 "-->" 的位置，因为解析 comment 需要找到 "-->" 结束标记
    if (string::npos == pos)              // pos 是找到的字符串的起始位置
    {
        return false;
    }

    m_idx = pos + 3;                      // + 3 是因为跳过 --> ，这样就相当于忽略了注释
    return true;
}

/* 解析节点名字 <students> 进入该函数时候 m_idx 指向 '<' ，结束时指向 '>' */
string Parser::parse_element_name()
{
    m_idx++;          // 从 '<' 开始，跳过尖括号
    int pos = m_idx;  // 此时指向 <students> 的第一个 s

    if (isalpha(m_str[m_idx]) || m_str[m_idx] == '_')  /* 要解析的第一个字符是否是字母或者下划线（防止第一个不符合要求） */
    {
        m_idx++;      // <students>
        while (isalnum(m_str[m_idx]))
        {             // isalnum 判断一个字符是否为字母或数字
            m_idx++;  // 只要是字母或者数字或者下划线或者-等字符都算该节点名
        }
    }

    /* 正常此时 m_idx 指向 '>' 
    也有可能指向名字后面的空格（因为后面如果跟属性的值的话还要去解析属性） */

    return m_str.substr(pos, m_idx - pos);    // 返回节点名称的字符串，从字符串中 pos 处截取子串长度为 m_idx-pos
}

/* 解析元素的文本 */
string Parser::parse_element_text()           // 假设文本是 <name>Jack</name>  pos -> 'J'
{
    m_idx++;                                  // 进入该函数时 m_idx 指向 '>'
    // 从 'J' 开始，如果此时从 > 进来的，如果不是文本就相当于 m_idx 加1，指向下一个 < 字符了
    int pos = m_idx;
    while (m_str[m_idx] != '<')
    {
        m_idx++;
    }
    /* 此时 m_idx 指向 '<' */

    return m_str.substr(pos, m_idx - pos);    // 如果传入一个位置pos，第二个参数是0，则返回空字符
}

/* 解析键值对的键 */
string Parser::parse_element_attr_key()       // <student id="1" class="101"> 此时 m_idx 指向 id 的 i
{
    int pos = m_idx;

    while (isalnum(m_str[m_idx]))             // 一直找键，m_idx 指向 = 或者字符结束的那个地方
    {
        m_idx++;
    }

    return m_str.substr(pos, m_idx - pos);
}

/* 解析键值对的值 */
string Parser::parse_element_attr_val()       // <student id="1" class="101"> 
{
    int pos = 0;

    if (m_str[m_idx] != '"')                  // 值肯定是字符 " 开始的
    {
        XML_LOG << "xml element attr val is error." << __LINE__;
        return "";
    }

    m_idx++;
    pos = m_idx;
    
    /* 直到双引号结束 */
    while (m_str[m_idx] != '"')
    {
        m_idx++;
    }

    m_idx++;    // 此时 m_idx 指向双引号后一个字符

    return m_str.substr(pos, m_idx - pos - 1);
}

/* public 外部可调的解析对象 */
Xml Parser::parse()
{
    /* 解析声明 */
    skip_white_space();             /* 先忽略空格等字符 */
    if (m_str.compare(m_idx, 5, "<?xml") == 0)      // 比较两个字符串是否相等
    {
        if (!parse_declaration())   /* 解析声明（实际就是跳过该声明） */
        {
            XML_LOG << "parse xml declaration error." << __LINE__;
            return Xml();           /* 返回空的匿名对象（其实做了一次拷贝构造） */
        }
    }

    /* 解析注释 */
    skip_white_space();
    while (m_str.compare(m_idx, 4, "<!--") == 0)    // 放在 while 中防止有多个注释
    {
        if (!parse_comment())       /* 如果是 <!-- 前缀代表这是注释，就去解析注释 */
        {
            XML_LOG << "parse xml comment error." << __LINE__;
            return Xml();
        }
        skip_white_space();         /* 注释之间万一有空格换行什么的 */
    }

    // 此时 m_idx 是指向 < 的位置，需要解析元素了
    /* 解析元素 */
    if (m_str[m_idx] == '<' && isalnum(m_str[m_idx + 1]))  /* 该元素首先是左括号开始，其次是一个字母或者数字 */
    {                               /* isalpha 判断字符是否为字母    isalnum 判断字符是否为字母或者数字 */
        Xml tmp = parse_element();
        // XML_LOG << tmp.name();
        // XML_LOG << tmp.text();
        return tmp;                 /* 解析正文内容，返回一个 XML 对象 */
    }

    XML_LOG << "parse element error." << __LINE__;
    return Xml();
}

/* 解析 XML 内容（这个函数就是 解析每一个节点） */
/* XML 节点示例：

<student id="1">
    <name>Jack</name>
</student>

这是最基本的 XML 格式（包含名字，属性，文本，子节点），按照这个解析，每个节点的解析方法相同，子节点就递归的去解析即可
*/
Xml Parser::parse_element()
{
    Xml elem;            // 定义一个解析后存放的 Xml 对象，用于返回

    skip_white_space();
    const string name = parse_element_name();   /* 解析节点名 */
    elem.name(name);     // 填充当前的名字（此时获取到该节点的名字）
    skip_white_space();  // 此时 m_idx 指向 <students> 的 '>' 或者 <student id="1"> 的 id 前面的空格，跳过该空格，指向 i

    // XML_LOG << elem.name();          // 测试语句

    /* 开始解析子节点 */
    while (m_str[m_idx] != '\0')        // 直到字符串结束或者内部逻辑结束了此次节点，也就是到了 </students>
    {
        // 如果除了 / 这个特殊情况外，其他的就是 < > 字母 三种字符了，分别是子节点，文本，属性
        skip_white_space();             // 确保循环的时候每次都会跳过换行空格
        /* 以 <students /> 方式结束（这个正常用不到，但是存在这种情况） */
        if (m_str[m_idx] == '/')
        {
            if (m_str[m_idx + 1] == '>')
            {
                m_idx += 2;
                break;
            }
            else
            {   /* 报异常 */
                XML_LOG << "parse close element error." << __LINE__;
                elem.clear();   // 清空当前对象，释放内存
                return Xml();   // 返回一个匿名对象
            }
        }
        /* 获取文本 text （当这个右尖括号代表的不是文本而是某个标签结束的右尖括号，则内部执行一次 m_idx++ 相当于指向下一个 < ） */
        else if (m_str[m_idx] == '>')                // 相当于解析到了 <students> 的 '>' ，而正常来说右尖括号后面跟的是文本内容，如果右尖括号右边不是文本而是左尖括号，则这个函数就直接退出了，m_idx 加1（就算是文本内容）推出该函数也指向 '<'
        {
            string text = parse_element_text();      // 解析文本内容（每个子函数进去时 m_idx 都指向左边尖括号 ，出来都指向右边尖括号）
            if (text != "")     // 如果获取的内容是空的则不执行赋值文本操作
            {
                elem.text(text);// 赋值此节点的文本内容
            }
            // XML_LOG << elem.text();
        }
        /* 解析子节点，如果遇到 < 则代表遇到新的节点，新的节点又要重复解一遍新的节点内容 */
        else if (m_str[m_idx] == '<')                // 左尖括号，后面可能是结束标记，可能是注释，都不是则代表是子节点名字的开始 
        {
            /* 防止子节点上来就是注释介绍 */
            if (m_str.compare(m_idx, 4, "<!--") == 0)
            {
                if (!parse_comment())   /* 执行完跳过注释就指向注释最后的>的下一个，正常是换行符 */
                {
                    XML_LOG << "parse comment error. " << __LINE__;
                    elem.clear();
                    return Xml();
                }
            }
            /* 左斜杠表示结束 */
            else if (m_str[m_idx + 1] == '/')        // </students> 此时 m_idx 指向 '<'
            {
                string end_tag;
                unsigned int pos = 0;

                /* 获取该节点的结束标记字符串 */
                end_tag = "</" + name + ">";         // 结束的标记 
                pos = m_str.find(end_tag, m_idx);    // 在字符串 m_str 中查找子串 end_tag，从 m_idx 开始查找
                if (string::npos == pos)
                {
                    XML_LOG << "parse close element error." << __LINE__;
                    elem.clear();
                    return Xml();
                }
                m_idx = pos + end_tag.size();        // 跳过该节点（包括子节点），此时指向 > 的下一个字符
                break;                               // 跳出当前循环表示当前节点已经遍历完成
            }
            /* 只有当正确进入到需要解析孩子节点的地方才会去递归调用依次解析，即 <name> */
            else
            {
                elem.append(parse_element());        // 递归调用本解析函数（ append 当前节点添加子节点）
            }
        }
        /* 获取属性的键值对（这个 else 是针对字母的，也就是空格后面就是属性的值了） */
        else if (isalnum(m_str[m_idx]))              // 代表后面是属性
        {
            string key;
            string val;

            key = parse_element_attr_key();          // <student id="1" class="101"> 进入该函数时指向id的i，出来的时候指向class前面的空格
            skip_white_space();                      // <student id = "1" class = "101"> 等号前后可能会有空格
            if (m_str[m_idx] != '=')
            {
                XML_LOG << "xml element attr is error." << __LINE__;
                elem.clear();
                return Xml();
            }
            m_idx++;
            skip_white_space();                      // <student id = "1" class = "101">
            val = parse_element_attr_val();          // 这个函数出来后我是将 m_idx 指向双引号后面的字符
            elem.attr(key, val);                     // 给该节点添加属性
        }
        else                                         // 不会执行这里，除非有问题，返回空串
        {
            XML_LOG << "parse error!" << __LINE__;
            return Xml();
        }
    }
    
    // XML_LOG << elem.name();
    // XML_LOG << elem.text();
    return elem;
}
