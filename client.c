#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8081
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
        error("Unable to create file.");
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
            fclose(file);
            error("Write to file failed");
        }
    }

    fclose(file);
    printf("%s has been received.\n", filename);
}

void upload_file(int socket_fd, char *filename)
{
    char filepath[256];
    size_t bytesRead;
    snprintf(filepath, sizeof(filepath), "%s/%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "rb");
    if (file == NULL)
    {
        error("Unable to open the file.");
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

char* findNewFileName(char *filename) {
    char *newFilename = malloc(strlen(filename) + 2);
    int t = 0;
    struct dirent *de;
    char name[256];
    char extension[256];
    strcpy(newFilename, filename);
    strcpy(name, filename);
    char *ext = strrchr(name, '.');
    if(ext != NULL){
        strcpy(extension, ext);
        *ext = '\0';
    }
    int a = 1;
    while(a) {
        a = 0;
        DIR *dr = opendir(FILE_PATH);
        while ((de = readdir(dr)) != NULL) {
            if (strcmp(de->d_name, newFilename) == 0) {
                a = 1;
                t++;
                if(ext != NULL){
                    sprintf(newFilename, "%s_%d%s", name, t, extension);
                }
                else {
                    sprintf(newFilename, "%s_%d", name, t);
                }
                break;
            }
        }
        closedir(dr);
    }
    return newFilename;
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

void listFilesRecursively(char *basePath, SendFileList *fileList, int *index)
{
    char path[1024];
    struct dirent *de;
    DIR *dr = opendir(basePath);
    size_t length_file_path = strlen(FILE_PATH);

    if (dr == NULL)
    {
        error("Không thể mở thư mục");
    }

    while ((de = readdir(dr)) != NULL)
    {
        if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
        {
            snprintf(path, sizeof(path), "%s/%s", basePath, de->d_name);

            if (de->d_type == DT_REG)
            {
                strncpy(fileList->filenames[*index], path, sizeof(fileList->filenames[*index]));
                strcpy(fileList->filenames[*index], fileList->filenames[*index] + length_file_path + 1);
                printf("%s\n", fileList->filenames[*index]);
                (*index)++;
            }
            else if (de->d_type == DT_DIR)
            {
                listFilesRecursively(path, fileList, index);
            }
        }
    }

    closedir(dr);
}

SendFileList listFilesUpload()
{
    SendFileList listFile;
    listFile.num_files = 0;
    printf("======== List files upload ======== \n");

    listFilesRecursively(FILE_PATH, &listFile, &(listFile.num_files));
    return listFile;
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
                char filepath[512];
                char filename[512];
                char *newfilename;
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

                newfilename = findNewFileName(filename);

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
                    receiveFile(socket_fd, newfilename);
                }
                else
                {
                    printf("File does not exist. Please try again.\n");
                }
                free(newfilename);
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
                    SendFileList listFile = listFilesUpload();
                    while (1)
                    {
                        char filepath[256];
                        char filename[256];
                        int a = 1;
                        printf("Enter the file name (file path) to upload or 'quit' to exit: ");
                        fgets(filepath, sizeof(filepath), stdin);
                        size_t len1 = strlen(filepath);
                        if (len1 > 0 && filepath[len1 - 1] == '\n')
                        {
                            filepath[len1 - 1] = '\0';
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

                        for (int i = 0; i < listFile.num_files; i++)
                        {
                            if (strcmp(filepath, listFile.filenames[i]) == 0)
                            {
                                a = 0;
                                socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                                connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
                                snprintf(request, sizeof(request), "%s%s", password, filename);
                                send(socket_fd, request, strlen(request) + 1, 0);
                                if (lets_upload(socket_fd) == 1){
                                        upload_file(socket_fd, listFile.filenames[i]);
                                        printf("Upload %s successfully.\n", filename);
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
