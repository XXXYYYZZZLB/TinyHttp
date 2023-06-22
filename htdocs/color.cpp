#include <iostream>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    char *data;
    char *length;
    char color[20];
    char c = 0;
    int flag = -1;

    std::cout << "Content-Type:text/html\r\n"
              << std::endl;
    std::cout << "\r\n"
              << std::endl;
    std::cout << "<html><title>color page</title>"
                 "<h1>Hello world</h1>"
                 "<h2>you are the best</h2>"
                 "<body><p>The page color is:"
              << std::endl;
    // GET
    if ((data = getenv("QUERY_STRING")) != NULL) // key=vlaue
    {
        while (*data != '=')
            data++;
        data++;
        sprintf(color, "%s", data);
    }
    // POST
    if ((length = getenv("CONTENT_LENGTH")) != NULL)
    {
        int i;
        for (i = 0; i < atoi(length); i++)
        {
            read(STDIN_FILENO, &c, 1);
            if (c == '=')
            {
                flag = 0;
                continue;
            }
            if (flag > -1)
            {
                color[flag++] = c;
            }
        }
        color[flag] = '\0';
    }
    std::cout << color << std::endl;
    std::cout << "<body style=\"background-color:" << color << "\"/>" << std::endl;
    std::cout << "</body></html>" << std::endl;
    return 0;
}