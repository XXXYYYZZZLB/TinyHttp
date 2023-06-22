/*****************************************************************************
 *  xxx xxx  Co., Ltd                                                         *
 *  Copyright (C) 2023 xxx xxx                                                *
 *  This program is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License version 3 as         *
 *  published by the Free Software Foundation.                                *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with OST. If not, see <http://www.gnu.org/licenses/>.               *
 *                                                                            *
 *  Unless required by applicable law or agreed to in writing, software       *
 *  distributed under the License is distributed on an "AS IS" BASIS,         *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,                             *
 *  either express or implied.                                                *
 *  See the License for the specific language governing permissions and       *
 *  limitations under the License.                                            *
 *                                                                            *
 *  @file     httpd.c                                                         *
 *  @brief    tinyhttp                                                        *
 *  Details.                                                                  *
 *                                                                            *
 *  @author   Devin                                                           *
 *  @email    libinnn2020@163.com                                             *
 *  @version  1.0.0.0                                                         *
 *  @date     2023-06-22                                                      *
 *  @license  GNU General Public License (GPL)                                *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *  Remark         : Description                                              *
 *----------------------------------------------------------------------------*
 *  Change History :                                                          *
 *   Date      |   Version | Author      | Description                        *
 *----------------------------------------------------------------------------*
 *  2023/06/21 |   1.0.0   | libinnn     | Create file                        *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *****************************************************************************/



#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SERVER_STRING "Server: lbhttpd/0.1.0\r\n"
#define ISspace(x) isspace((int)(x)) // int isspace(int c) 检查所传的字符是否是空白字符
                                     // 如果 c 是一个空白字符，则该函数返回非零值（true），否则返回 0（false）
                                     //  标准的空白字符包括：' ' '\t' '\n' '\v' '\f' '\r'

void accept_request(int);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

/** 
 * @brief 出错处理函数
 * @param sc    perror(sc)
 * @return void
 */
void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}

/** 
 * @brief 创建socket监听
 * @param port    绑定的端口号，0时为自动分配
 * @return 绑定的端口号
 */
int startup(u_short *port)
{
    printf("startup start!\n");
    int httpd_socket = 0;
    struct sockaddr_in addr;
    httpd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (!httpd_socket)
        error_die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(*port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY是一个 IPV4通配地址的常量
    if (bind(httpd_socket, (struct sockaddr *)&addr, sizeof(addr)))
        error_die("bind");

    if (*port == 0)
    {
        int addr_len = sizeof(addr);
        if (getsockname(httpd_socket, (struct sockaddr *)&addr, &addr_len) == -1)
            error_die("getsockname");
        *port = ntohs(addr.sin_port);
    }

    if (listen(httpd_socket, 5) < 0)
        error_die("listen");

    printf("startup done!\n");
    return httpd_socket;
}

/** 
 * @brief 读取文件描述符sock的一行数据，读取到buf中
 * @param sock  套接字(文件描述符)
 * @param buf   缓冲区
 * @param size  缓冲区大小
 * @return buf的长度
 */
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0); // 读一个字节的数据存放在 c 中
        if (n > 0)                // 接收到了数据
        {
            if (c == '\r') // 成对出现\r\n
            {
                n = recv(sock, &c, 1, MSG_PEEK); // MSG_PEEK时代表只是查看数据，而不取走数据
                if ((n > 0) && (c == '\n'))      // 判断是不是\r\n
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n'; // 此时就是没有数据了
    }
    buf[i] = '\0'; // buf[i]里的 \r\n 只保存了 \n

    return i;
}


/** 
 * @brief 501未实现处理函数
 * @param client  客户端套接字(文件描述符)
 * @return void
 */
void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/** 
 * @brief 404处理函数
 * @param client  客户端套接字(文件描述符)
 * @return void
 */
void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/** 
 * @brief header发送函数
 * @param client  客户端套接字(文件描述符)
 * @param filename 发送的文件名(没什么用)
 * @return void
 */
void headers(int client, const char *filename)
{
    char buf[1025];
    (void)filename;
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/** 
 * @brief 紧接着header后需要跟着发送的数据
 * @param client  客户端套接字(文件描述符)
 * @param resource 要发送的资源FILE*
 * @return void
 */
void cat(int client, FILE *resource)
{
    char buf[1024];

    // 从文件文件描述符中读取指定内容
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

/** 
 * @brief 不携带数据的时候返回页面
 * @param client  客户端套接字(文件描述符)
 * @param filename 要发送的资源文件名
 * @return void
 */
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    // 此时就不需要头部字段了
    //  确保 buf 里面有东西，能进入下面的 while 循环
    buf[0] = 'A';
    buf[1] = '\0';
    // 循环作用是读取并忽略掉这个 http 请求后面的所有内容
    while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));

    resource = fopen(filename, "r");
    if (resource == NULL)
    {
        not_found(client);
    }
    else
    {
        // 打开成功后，将这个文件的基本信息封装成 response 的头部(header)并返回
        headers(client, filename);
        // 接着把这个文件的内容读出来作为 response 的 body 发送到客户端
        cat(client, resource);
    }
}

/** 
 * @brief 400错误的请求处理
 * @param client  客户端套接字(文件描述符)
 * @return void
 */
void bad_request(int client)
{
  char buf[1024];

  sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "Content-type: text/html\r\n");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "<P>Your browser sent a bad request, ");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "such as a POST without a Content-Length.\r\n");
  send(client, buf, sizeof(buf), 0);
}

/** 
 * @brief 500服务器内部错误处理
 * @param client  客户端套接字(文件描述符)
 * @return void
 */
void cannot_execute(int client)
{
  char buf[1024];

  sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
  send(client, buf, strlen(buf), 0);
}

/*
例
POST /htdocs/index.html HTTP/1.1    \r\n
Host:www.baidu.com                  \r\n
Content-Length:9                    \r\n
\r\n
color=red
*/
/** 
 * @brief cgi调用程序，包含前置处理和生成子进程
 * @param client            客户端套接字(文件描述符)
 * @param path              cgi路径，请求路径
 * @param method            请求方法
 * @param query_string      如果是get，提取出的？后字符
 * @return void
 */
void execute_cgi(int client, const char *path,
                 const char *method, const char *query_string)//query_string(GET)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    // 往 buf 中填东西以保证能进入下面的 while
    buf[0] = 'A';
    buf[1] = '\0';
    // 如果是 http 请求是 GET 方法的话读取并忽略请求剩下的内容
    if (strcasecmp(method, "GET") == 0)
        while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
    else /* POST */
    {
        // 只有 POST 方法才继续读内容
        numchars = get_line(client, buf, sizeof(buf));
        // 这个循环的目的是读出指示 body 长度大小的参数，并记录 body 的长度大小。其余的 header 里面的参数一律忽略
        // 注意这里只读完 header 的内容，body 的内容没有读
        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16])); // 记录 body 的长度大小
            numchars = get_line(client, buf, sizeof(buf));
        }

        // 如果 http 请求的 header 没有指示 body 长度大小的参数，则报错返回
        if (content_length == -1)
        {
            bad_request(client);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    // 下面这里创建两个管道，用于两个进程间通信
    if (pipe(cgi_output) < 0)
    {
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0)
    {
        cannot_execute(client);
        return;
    }

    // 创建一个子进程
    if ((pid = fork()) < 0)
    {
        cannot_execute(client);
        return;
    }

    // 子进程用来执行 cgi 脚本
    if (pid == 0) /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        // 将子进程的输出由标准输出重定向到 cgi_ouput 的管道写端上
        dup2(cgi_output[1], 1);
        // 将子进程的输出由标准输入重定向到 cgi_ouput 的管道读端上
        dup2(cgi_input[0], 0);
        // 关闭 cgi_ouput 管道的读端与cgi_input 管道的写端
        close(cgi_output[0]);
        close(cgi_input[1]);

        // 构造一个环境变量(子进程中)
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        // 将这个环境变量加进子进程的运行环境中
        putenv(meth_env);

        // 根据http 请求的不同方法，构造并存储不同的环境变量
        if (strcasecmp(method, "GET") == 0)
        {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else
        { /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }

        // 最后将子进程替换成另一个进程并执行 cgi 脚本
        execl(path, path, NULL);
        exit(0);
    }
    else
    { /* parent */
        // 父进程则关闭了 cgi_output管道的写端和 cgi_input 管道的读端
        close(cgi_output[1]);
        close(cgi_input[0]);

        // 如果是 POST 方法的话就继续读 body 的内容，并写到 cgi_input 管道里让子进程去读
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++)
            {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }

        // 然后从 cgi_output 管道中读子进程的输出，并发送到客户端去
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        // 关闭管道
        close(cgi_output[0]);
        close(cgi_input[1]);
        // 等待子进程的退出
        waitpid(pid, &status, 0);
    }
}

/**
HTTP请求格式

请求方法 空格 URL 空格 协议版本 回车符 换行符    (请求行)
头部字段名称 : 值 回车符 换行符                 (请求头部)
头部字段名称 : 值 回车符 换行符
...
回车符 换行符
(请求正文)

 */
void accept_request(int client)
{
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;
    int cgi = 0; // becomes true if server decides this is a CGI program

    // 读http请求的第一行数据 记录 (请求行)
    numchars = get_line(client, buf, sizeof(buf));

    i = 0;
    j = 0;
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    printf("%s\n",buf);
    printf("METHOD: %s\n", method);

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) // 如果请求的方法不是 GET 或 POST 任意一个的话就直接发送 response 告诉客户端没实现该方法
    {
        unimplemented(client);
        return;
    }

    if (strcasecmp(method, "POST") == 0) // 如果是 POST 方法就将 cgi 标志变量置一(true) 调用cgi函数
        cgi = 1;

    /* 提取URL */
    i = 0;

    while (ISspace(buf[j]) && (j < sizeof(buf))) // 跳过所有的空白字符(空格)
        j++;

    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) // 然后把 URL 读出来放到 url 数组中
    {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    /* 如果这个请求是一个 GET 方法 */
    /* 例 127.0.0.1:8888/color.cgi?color=red */
    char *query_string = NULL;
    if (strcasecmp(method, "GET") == 0)
    {
        // 用一个指针指向 url
        query_string = url;

        // 去遍历这个 url，跳过字符 ？前面的所有字符，如果遍历完毕也没找到字符 ？则退出循环
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;

        // 退出循环后检查当前的字符是 ？还是字符串(url)的结尾
        if (*query_string == '?')
        {
            cgi = 1;              // 如果是 ？ 的话，证明这个请求需要调用 cgi，将 cgi 标志变量置一(true)
            *query_string = '\0'; // 从字符 ？ 处把字符串 url 给分隔会两份
            query_string++;       // 使指针指向字符 ？后面的那个字符
        }
    }

    // 将前面分隔两份的前面那份字符串，拼接在字符串htdocs的后面之后就输出存储到数组 path 中。相当于现在 path 中存储着一个字符串
    sprintf(path, "htdocs%s", url);

    // 如果 path 数组中的这个字符串的最后一个字符是以字符 / 结尾的话，就拼接上一个"index.html"的字符串。首页的意思
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");

    if (stat(path, &st) == -1) // 在系统上去查询该文件是否存在
    {
        // 如果不存在，那把这次 http 的请求后续的内容(head 和 body)全部读完并忽略
        while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        not_found(client); // 然后返回一个找不到文件的 response 给客户端
    }
    else
    {

        if ((st.st_mode & S_IFMT) == S_IFDIR) // 文件存在，那去跟常量S_IFMT相与，相与之后的值可以用来判断该文件是什么类型的
            strcat(path, "/index.html");      // 如果这个文件是个目录，那就需要再在 path 后面拼接一个"/index.html"的字符串

        if ((st.st_mode & S_IXUSR) ||
            (st.st_mode & S_IXGRP) ||
            (st.st_mode & S_IXOTH))
            // 如果这个文件是一个可执行文件，不论是属于用户/组/其他这三者类型的，就将 cgi 标志变量置一
            cgi = 1;

        if (!cgi)
            serve_file(client, path); // 如果不需要 cgi 机制的话，
        else
            execute_cgi(client, path, method, query_string); // 如果需要则调用
    }

    close(client);
}

int main()
{
    int server_sock = -1;
    u_short port = 8888;
    int client_sock = -1;

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    server_sock = startup(&port); // 返回监听到的socket 和 得到端口号
    printf("httpd running on port %d\n", port);

    while (1)
    {
        // 阻塞等待客户端连接
        printf("wait accept...\n");
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1)
            error_die("accept");

        accept_request(client_sock);
    }

    close(server_sock);

    return 0;
}