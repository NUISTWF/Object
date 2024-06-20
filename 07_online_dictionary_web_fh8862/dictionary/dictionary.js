function typefunc(action)
{
    var sendDate = "";
    sendDate += (document.getElementById("Username").value);
    sendDate += "-";
    sendDate += (document.getElementById("Password").value);
    sendDate += "-";
    sendDate += (document.getElementById("Word").value);
    sendDate += "-";
    if (0 == action) {
        sendDate += "register";    // sendDate = xm-123-impute-register
    } else if (1 == action) {
        sendDate += "login";
    } else if (2 == action) {
        sendDate += "query";
    } else if (3 == action) {
        sendDate += "logout";
    } else if (4 == action) {
        sendDate += "history";
    }

    loadDate(sendDate);  // 发送该字符串
}

// 创建一个新的 XMLHttpRequest 对象
function get_xmlreq()
{
    var xhr = new XMLHttpRequest();

    return xhr;
}

//  处理请求
function loadDate(sendDate)
{
    var url = "/cgi-bin/dictionary.cgi";

    //创建 XMLHttpRequest 对象
    var xhr = get_xmlreq();

    // 定义请求完成时的回调函数
    xhr.onreadystatechange = function()
    {
        if (xhr.readyState === 4) { // 请求完成
            if (xhr.status === 200) { // 请求成功
                var ret = xhr.responseText;
                document.getElementById("result").innerHTML = ret; // 将内容输出到我html中定义的结果标签中显示
                console.log(xhr.responseText); // 控制台输出响应内容
            } else {
                console.error("请求失败，状态码: " + xhr.status);
            }
        }
    }

    // 初始化一个 POST 请求
    xhr.open("POST", url, true);

    // 清理缓存
    xhr.setRequestHeader("If-Modified-Since", "0");

    // 发送请求
    xhr.send(sendDate);
}