# 1、XML的生成与解析

## 1.1 TinyXML库

TinyXML是一个轻量级的C++ XML解析器库，它提供了简单的接口来解析和生成XML文档。TinyXML易于使用，特别适合于嵌入式系统和那些对性能有严格要求的场合。 

TinyXML的一些关键特点： 

- 轻量级：TinyXML没有依赖外部库，它的代码量很小，易于集成到其他项目中。 
- 易于使用：TinyXML提供了简单的API来创建、解析和查询XML文档。
- 解析XML：它支持解析XML文档，包括解析元素、属性、文本内容、注释、处理指令等。 
- 生成XML：TinyXML也可以用来生成XML文档。 
- 流式解析：TinyXML支持流式解析，可以边读取边处理XML文档。

除了此库，还有一些其他解析XML的库可以使用；此处举例说明而已。



## 1.2 生成和解析XML

功能（本代码中已实现的，普遍使用较多的）：

1. 从文件加载 XML
2. 从字符串加载 XML
3. 访问节点：根据数组下标、节点名称
4. 遍历节点：数组、迭代器
5. 添加节点
6. 删除节点
7. 获取、修改节点名称
8. 获取、修改节点属性
9. 获取、修改节点内容





## 1.3 编译测试

编译example的xml示例：

```bash
make
```

编译客户端：

```bash
g++ http_client.cpp Xml.cpp -o http_client -L./ -lcurl -lnghttp2 -lssl -lcrypto
```

编译服务器：

```bash
python3.6 http_server.py
```









