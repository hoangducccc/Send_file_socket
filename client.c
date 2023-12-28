#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define FILE_PATH "files_client/"

void error(char *message)
{
    perror(message);
    exit(1);
}

typedef struct
{
    int num_files;
    char filenames[100][256];
} SendFileList;

void receiveFile(int socket_fd, char *filename)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "wb");
    if (file == NULL)
    {
        perror("Unable to create file.");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    ssize_t bytesRead;

    while (1)
    {
        bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
        {
            break;
        }
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, file);
        if (bytesWritten != bytesRead)
        {
            perror("Write to file failed");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    printf("%s has been received.\n", filename);
}

void upload_file(int socket_fd, char *filename)
{
    char filepath[256];
    size_t bytesRead;
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "rb");
    if (file == NULL)
    {
        perror("Unable to open the file.");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        ssize_t bytesSent = send(socket_fd, buffer, bytesRead, 0);
        if (bytesSent == -1)
        {
            perror("Error while sending data.");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        if ((size_t)bytesSent != bytesRead)
        {
            perror("Not enough data sent.");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
}

SendFileList listFiles(int socket_fd)
{
    char request[] = "list";
    send(socket_fd, request, strlen(request) + 1, 0);

    SendFileList receiveList;
    if (recv(socket_fd, &receiveList, sizeof(receiveList), 0) == -1)
    {
        error("Error while receiving the list of files.");
    }

    printf("======= List files on server =======\n");
    for (int i = 0; i < receiveList.num_files; i++)
    {
        printf("%s\n", receiveList.filenames[i]);
    }

    return receiveList;
}

int upload_confirm(int socket_fd)
{
    char buffer[1024];
    ssize_t bytesRead;
    int t = 1;

    bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        close(socket_fd);
    }
    if (strcmp(buffer, "agree_to_upload") == 0)
    {
        return 1;
    }
    else if (strcmp(buffer, "refuse_to_upload") == 0)
    {
        return 0;
    }
    close(socket_fd);
}

int lets_upload(int socket_fd)
{
    char buffer[1024];
    ssize_t bytesRead;
    int t = 1;

    bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        close(socket_fd);
    }
    if (strcmp(buffer, "let's_upload") == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



char **list_files_upload(int *num_files)
{
    struct dirent *de;
    DIR *dr = opendir(FILE_PATH);

    if (dr == NULL)
    {
        error("Unable to open the folder");
    }

    *num_files = 0;
    char **fileList = NULL;

    printf("======== List files on server ========\n");

    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG)
        {
            *num_files += 1;
            fileList = (char **)realloc(fileList, sizeof(char *) * (*num_files));
            fileList[*num_files - 1] = strdup(de->d_name);

            printf("%s\n", de->d_name);
        }
    }

    closedir(dr);

    return fileList;
}

void clearInputBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main()
{
    int socket_fd;
    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    while (1)
    {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            error("Connection to the server failed.");
        }

        printf("====== Please choose a service =====\n");
        printf("1. List file\n");
        printf("2. Download\n");
        printf("3. Upload\n");
        printf("4. Exit\n");

        int choice;
        scanf("%d", &choice);
        clearInputBuffer();

        switch (choice)
        {
        case 1:
        {
            SendFileList receiveList = listFiles(socket_fd);
            break;
        }
        case 2:
        {
            int t = 0;
            char request[1024] = "list";
            SendFileList receiveList = listFiles(socket_fd);
            close(socket_fd);
            while (1)
            {
                char filepath[256];
                char filename[256];
                printf("Enter the file name (file path) to download or 'quit' to exit: ");
                fgets(filepath, sizeof(filepath), stdin);

                size_t len = strlen(filepath);
                if (len > 0 && filepath[len - 1] == '\n')
                {
                    filepath[len - 1] = '\0';
                }

                if (strcmp(filepath, "quit") == 0)
                {
                    break;
                }

                const char *lastSlash = strrchr(filepath, '/');
                if (lastSlash != NULL) {
                        strcpy(filename, lastSlash + 1);
                }
                else {
                        strcpy(filename, filepath);
                }

                int find = 0;
                for (int i = 0; i < receiveList.num_files; i++)
                {
                    if (strcmp(receiveList.filenames[i], filepath) == 0)
                    {
                        find = 1;
                        break;
                    }
                }

                if (find)
                {
                    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
                    snprintf(request, sizeof(request), "%s%s", "send", filepath);
                    send(socket_fd, request, strlen(request) + 1, 0);
                    receiveFile(socket_fd, filename);
                }
                else
                {
                    printf("File does not exist. Please try again.\n");
                }
            }
            break;
        }

        case 3:
        {
            int b = 1;
            while(b) {
                char request[1024] = "upload";
                socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
                char password[256];
                printf("Enter the password or 'quit' to exit: ");
                fgets(password, sizeof(password), stdin);

                size_t len = strlen(password);
                if (len > 0 && password[len - 1] == '\n')
                {
                    password[len - 1] = '\0';
                }

                if (strcmp(password, "quit") == 0)
                {
                    break;
                }

                snprintf(request, sizeof(request), "%s%s", "upload", password);
                send(socket_fd, request, strlen(request) + 1, 0);
                if (upload_confirm(socket_fd) == 1)
                {
                    b = 0;
                    //close(socket_fd);

                    //int a = 1;
                    int num_files;
                    char **fileList = list_files_upload(&num_files);
                    while (1)
                    {
                        char filename[256];
                        int a = 1;
                        printf("Enter the file name to upload or 'quit' to exit: ");
                        fgets(filename, sizeof(filename), stdin);
                        size_t len1 = strlen(filename);
                        if (len1 > 0 && filename[len1 - 1] == '\n')
                        {
                            filename[len1 - 1] = '\0';
                        }

                        if (strcmp(filename, "quit") == 0)
                        {
                            break;
                            //a = 0;
                        }

                        for (int i = 0; i < num_files; i++)
                        {
                            if (strcmp(filename, fileList[i]) == 0)
                            {
                                a = 0;
                                socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                                connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
                                snprintf(request, sizeof(request), "%s%s", password, fileList[i]);
                                send(socket_fd, request, strlen(request) + 1, 0);
                                if (lets_upload(socket_fd) == 1){
                                        upload_file(socket_fd, fileList[i]);
                                        printf("Upload %s successfully.\n", fileList[i]);
                                }
                                close(socket_fd);
                                break;
                            }
                        }
                        if (a == 1)
                        {
                            printf("File does not exist. Please try again.\n");
                        }
                    }
                }
                else
                {
                    printf("Incorrect password, please enter again.\n");
                }
            }

            break;
        }

        case 4:
            exit(0);
            break;
        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
        close(socket_fd);
    }


    return 0;
}
