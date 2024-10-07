#include <stdio.h>
#include "curl.h"
#include "Xml.h"

using namespace std;
using namespace hik_wangfei105::xml;

int main()
{
    CURL *curl;
    CURLcode res;
    Xml root;

    /* ########## 设计一个 XML 对象 ########## 
    示例：（调用自己写的 XML 库）

    <student id="1" class="101">
        <name>Jack</name>
        <age>22</age>
        <gender>male</gender>
    </student>
    
    */

    Xml name;
    name.name("name");
    name.text("Jack");

    Xml age;
    age.name("age");
    age.text("22");

    Xml gender;
    gender.name("gender");
    gender.text("male");

    root.name("student");
    root.attr("id", "1");

    root.append(name);
    root.append(age);
    root.append(gender);

    // 序列化
    string str = root.serialization();

    // 初始化
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://10.1.65.184:8000");

        // 请求方法POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // 设置要发送的数据（XML格式）
        const char *xml_data = str.c_str();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml_data);

        // 执行HTTP请求
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // 清理libcurl
        curl_easy_cleanup(curl);
    }

    // 清理libcurl的全局设置
    curl_global_cleanup();

    return 0;
}
