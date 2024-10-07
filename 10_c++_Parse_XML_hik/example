#include "Xml.h"

using namespace std;
using namespace hik_wangfei105::xml;

int main()
{
    /* ########## 设计一个 XML 对象 ########## 
    示例：

    <student id="1" class="101">
        <name>Jack</name>
        <age>22</age>
        <gender>male</gender>
    </student>
    
    */

    Xml root;

    Xml name;
    name.name("name");
    name.text("Jack");

    Xml age;
    age.name("age");
    age.text("22");

    Xml gender;
    gender.name("gender");
    gender.text("male");

    root.name("student");   /* 设计根节点的节点名 */
    root.attr("id", "1");   /* 设计节点属性 */

    root.append(name);
    root.append(age);       /* 添加一个年龄子节点 */
    root.append(gender);    /* 添加一个性别子节点 */

    string str = root.serialization();   /* 序列化 */

    cout << str << endl;    /* 打印序列化后的 XML 对象变为字符串 */



    cout << "======================================================================" << endl;
    /* ########## 解析字符串为一个 XML 对象 ########## */

    Xml res;

    string s1 = "<student id=\"1\">hik_wangfei105</student>";
    res.parse(s1);         /* 解析 XML 字符串保存到xml对象res中 */

    cout << res.name() << endl;
    cout << res.attr("id") << endl;
    cout << res.text() << endl;
    
    cout << res.serialization() << endl;   /* 序列化 */



    cout << "======================================================================" << endl;
    /* ########## 解析一个xml文件为一个 XML 对象 ########## */

    Xml fd;

    fd.load("test.xml");
    fd.save("test_copy.xml");   // 将xml保存到文件中

    cout << fd.serialization() << endl;   /* 序列化 */



    cout << "======================================================================" << endl;
    /* ########## 迭代器和数组方式获取指定元素（此段可以封装为一个函数，传入指定的参数即可） ########## */

    /* 迭代器的方式获取子节点信息（比如说下方的获取 id 为 1 同学的年龄） */
    for (Xml::iterator it = fd.begin(); it != fd.end(); it++)
    {
        if ((*it).attr("id") == "1")
        {
            for (Xml::iterator it1 = (*it).begin(); it1 != (*it).end(); it1++)
            {
                if ((*it1).name() == "age")
                {
                    cout << "id=1 age:" << (*it1).text() << endl;
                    break;
                }
            }
        }
    }


    /* 下标的方式获取子节点信息（比如说下方的获取 class 为 102 同学的性别） */
    for (int i = 0; i < fd.size(); i++)
    {
        if (fd[i].attr("class") == "102")
        {
            for (int j = 0; j < fd[i].size(); j++)
            {
                if (fd[i][j].name() == "gender")        /* （fd[i]就是一个xml对象）因为[]重载时返回引用支持链式编程，所有可以直接往后叠加 [] */
                {
                    cout << "class=102 gender:" << fd[i][j].text() << endl;
                    break;
                }
            }
        }
    }

    return 0;
}
