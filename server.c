#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080
#define FILE_PATH "files_server/"
#define PASSWORD "nhom1"

typedef struct
{
    int num_files;
    char filenames[100][256];
} SendFileList;

void sendFile(int client_socket, char *filename)
{
    char filepath[256];
    size_t bytesRead;
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "rb");
    if (file == NULL)
    {
        perror("Unable to open the file.");
    }

    char buffer[1024];

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        ssize_t bytesSent = send(client_socket, buffer, bytesRead, 0);
        if (bytesSent == -1)
        {
            perror("Error while sending data.");
            fclose(file);
        }
        if ((size_t)bytesSent != bytesRead)
        {
            perror("Not enough data sent.");
            fclose(file);
        }
    }

    fclose(file);
}

void listFilesRecursively(char *basePath, SendFileList *fileList, int *index)
{
    char path[1024];
    struct dirent *de;
    DIR *dr = opendir(basePath);
    size_t length_file_path = strlen(FILE_PATH);

    if (dr == NULL)
    {
        perror("Can't open folder");
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

void listFiles(int client_socket)
{
    SendFileList fileList;
    fileList.num_files = 0;

    listFilesRecursively(FILE_PATH, &fileList, &(fileList.num_files));

    if (send(client_socket, &fileList, sizeof(fileList), 0) == -1)
    {
        perror("Error");
    }
}

char* findNewFileName(char *filename) {
    char *newFilename = malloc(strlen(filename) + 5);
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


void receiveFile(int client_socket, char *filename)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "wb");
    if (file == NULL)
    {
        perror("Unable to create file.");
    }

    char buffer[1024];
    ssize_t bytesRead;

    while (1)
    {
        bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
        {
            break;
        }
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, file);
        if (bytesWritten != bytesRead)
        {
            perror("Write to file failed");
            fclose(file);
            break;
        }
    }

    fclose(file);
}

void uploadConfirm(int client_socket)
{
    char request[] = "agree_to_upload";
    send(client_socket, request, strlen(request) + 1, 0);
}


void uploadRefuse(int client_socket)
{
    char request[] = "refuse_to_upload";
    send(client_socket, request, strlen(request) + 1, 0);
}

void letsUpload(int client_socket)
{
    char request[] = "let's_upload";
    send(client_socket, request, strlen(request) + 1, 0);
}

void *client_handler(void *arg)
{
    int client_socket = *((int *)arg);
    char buffer[1024];
    ssize_t bytesRead;
    size_t length_password = strlen(PASSWORD);

    bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        close(client_socket);
        pthread_exit(NULL);
    }

    if (strcmp(buffer, "list") == 0)
    {
        listFiles(client_socket);
    }
    else if (strncmp(buffer, "send", 4) == 0)
    {
        strcpy(buffer, buffer + 4);
        sendFile(client_socket, buffer);
    }
    else if (strncmp(buffer, "upload", 6) == 0)
    {
        strcpy(buffer, buffer + 6);
        if (strncmp(buffer, PASSWORD, length_password) == 0)
        {
            strcpy(buffer, buffer + length_password);
            if (strlen(buffer) == 0)
            {
                uploadConfirm(client_socket);
            }
        }
        else
        {
            uploadRefuse(client_socket);
        }
    }
    else if (strncmp(buffer, PASSWORD, length_password) == 0)
    {
        strcpy(buffer, buffer + length_password);
        char *newfilename = findNewFileName(buffer);
        letsUpload(client_socket);
        receiveFile(client_socket, newfilename);
        free(newfilename);
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main()
{
    int socket_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("Unable to create socket.");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Unable to bind the socket.");
        exit(1);
    }

    if (listen(socket_fd, 5) == -1)
    {
        perror("Unable to listen for connections.");
        exit(1);
    }

    printf("Waiting for connection...\n");

    while (1)
    {
        client_socket = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            perror("Unable to accept connection.");
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, &client_socket) != 0)
        {
            perror("Unable to create a new thread.");
        }

        pthread_detach(thread);
    }

    close(socket_fd);

    return 0;
}
