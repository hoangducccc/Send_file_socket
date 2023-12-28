# Send_file
Tải code chương trình về
```
git clone https://github.com/hoangducccc/Send_file_socket
```
Có thể thay đổi địa chỉ ip của server tùy thuộc vào nhu cầu sử dụng, mặc định trong chương trình client.c để địa chỉ loopback để thực hiện test trên một máy
```
#define SERVER_IP "127.0.0.1"
```
Có thể thay đổi password trong server.c tại 
```
#define PASSWORD "nhom1"
```
Tại server các file muốn chia sẻ với client được lưu tại "files_server"
Tại client các file tải về từ server và upload lên server lưu tại "files_client"
Có thể thay đổi đường dẫn thư mục muốn lưu trữ trong chương trình client.c và server.c tại đây:
```
#define FILE_PATH "files_server/"
```
Biên dịch chương trình từ server.c
```
gcc -o server server.c -pthread
```
Chạy chương trình bên server
```
./server
```
Biên dịch chương trình từ client.c
```
gcc -o client client.c
```
Chạy chương trình bên server
```
./client
```
