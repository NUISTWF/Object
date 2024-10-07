# -*- coding: utf-8 -*-
from http.server import HTTPServer, BaseHTTPRequestHandler
import socketserver

class RequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # 发送响应状态行
        self.send_response(200)
        # 发送头部信息
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        
        # 发送一些内容
        self.wfile.write(b"Hello, this is a simple HTTP server running on port 8000!")

    def do_POST(self):
        # 发送响应状态行
        self.send_response(200)
        # 发送头部信息
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        
        # 读取请求体
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        
        # 处理POST数据
        print("Received data: ", post_data.decode('utf-8'))
        data = post_data.decode('utf-8')
        print(data)
        return data

        # 发送响应
        self.wfile.write(b"Data received and processed successfully!")

# 设置服务器地址和端口
server_address = ('', 8000)

# 创建HTTP服务器实例 ( http://10.1.65.184:8000 )
httpd = HTTPServer(server_address, RequestHandler)

print("Serving HTTP on port 8000...")
# 启动服务器
httpd.serve_forever()
