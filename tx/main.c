#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>

#pragma pack(1)
struct File
{
    uint32_t size;
    char name[13];
    char unused[15];
};

int main(int argc, char** argv)
{
    SOCKET client;
    SOCKADDR_IN address;
    int connectresult;
    WSADATA wsaData;
    int result;

    const uint16_t Port = htons(2000);

    if (argc < 3)
    {
        printf("usage: tx <ip address> <filename1> <filename2> ..\n");
        return 0;
    }

    int haserrors = 0;
    int numfiles = argc - 2;
    for (int i = 0; i < numfiles; i++)
    {
        if (strlen(argv[i+2]) > 12)
        {
            printf("filename '%s' is too long. filename should be in 8.3 format\n", argv[i]);
            haserrors = 1;
        }
    }

    if (haserrors)
        return 0;

    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != NO_ERROR)
    {
        printf("WSAStartup() failed (%d)", result);
        return 1;
    }

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET)
    {
        printf("socket() failed\n");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_port = Port;
    address.sin_addr.s_addr = inet_addr(argv[1]);
    connectresult = connect(client, (SOCKADDR*)&address, sizeof(address));
    if (connectresult == SOCKET_ERROR)
    {
        printf("connect() failed\n");
        return 1;
    }

    printf("tx connected..\n");
    for (int i = 0; i < numfiles; i++)
    {
        struct File file;
        sprintf(file.name, "%s", argv[i+2]);
        FILE* fp = fopen(file.name, "rb");
        fseek(fp, 0, SEEK_END);
        file.size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        send(client, (char*)&file, sizeof(file), 0);

        // Send file contents.
        char buffer[8192];
        while (1)
        {
            long pos = ftell(fp);
            int progress = (pos*100) / file.size;
            printf("\r%s, %i bytes - %i%%", file.name, file.size, progress);
            int numread = fread(buffer, 1, sizeof(buffer), fp);
            if (numread > 0)
            {
                send(client, buffer, numread, 0);
            }
            else
            {
                Sleep(1000);
                printf("\n");
                break;
            }
        }
    }

    Sleep(1000);
    closesocket(client);

    WSACleanup();

    return 0;
}
