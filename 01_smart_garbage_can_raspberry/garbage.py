# -*- coding: utf-8 -*-
# 引入依赖包
# pip install alibabacloud_imagerecog20190930

import os
import io
from urllib.request import urlopen
from alibabacloud_imagerecog20190930.client import Client
from alibabacloud_imagerecog20190930.models import ClassifyingRubbishAdvanceRequest
from alibabacloud_tea_openapi.models import Config
from alibabacloud_tea_util.models import RuntimeOptions

config = Config(
  # 创建AccessKey ID和AccessKey Secret，请参考https://help.aliyun.com/document_detail/175144.html。
  # 如果您用的是RAM用户的AccessKey，还需要为RAM用户授予权限AliyunVIAPIFullAccess，请参考https://help.aliyun.com/document_detail/145025.html
  # 从环境变量读取配置的AccessKey ID和AccessKey Secret。运行代码示例前必须先配置环境变量。
  access_key_id=os.environ.get('ALIBABA_CLOUD_ACCESS_KEY_ID'),
  access_key_secret=os.environ.get('ALIBABA_CLOUD_ACCESS_KEY_SECRET'),
  # 访问的域名
  endpoint='imagerecog.cn-shanghai.aliyuncs.com',
  # 访问的域名对应的region
  region_id='cn-shanghai'
)

def alibaba_garbage():  #封装成函数方便调用
    #场景一：文件在本地
    img = open(r'/home/pi/wf/me/python-rubbish/Rubbish3.jpg', 'rb')
    #场景二：使用任意可访问的url
    #url = 'https://viapi-test-bj.oss-cn-beijing.aliyuncs.com/viapi-3.0domepic/imagerecog/ClassifyingRubbish/ClassifyingRubbish1.jpg'
    #img = io.BytesIO(urlopen(url).read())
    classifying_rubbish_request = ClassifyingRubbishAdvanceRequest()
    classifying_rubbish_request.image_urlobject = img
    runtime = RuntimeOptions()
    try:
      # 初始化Client
      client = Client(config)
      response = client.classifying_rubbish_advance(classifying_rubbish_request, runtime)
      # 获取整体结果
      # print(response.body.to_map())   # 加个.to_map()表示将输出的数据转为python的字典类型
      # print(response.body.to_map()['Data']['Elements'][0]['Category'])  # 直接只打印干垃圾
      return response.body.to_map()['Data']['Elements'][0]['Category']  # 返回给c语言的就是干垃圾
    except Exception as error:
      #报错打印我想打印的东西
      return '获取失败'
      # 获取整体报错信息
      print(error)
      # 获取单个字段
      print(error.code)

#if __name__ == "__main__":
#    alibaba_garbage()