# Send_file
## Cài đặt chương trình
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
  Chạy chương trình bên client
```
./client
```
## Hướng dẫn sử dụng chương trình
Sau khi client kết nối với server sẽ hiện ra danh sách các dịch vụ để người dùng chọn, nhập số tương ứng với dịch vụ muốn sử dụng.
### Tính năng List file
Khi nhập 1 sẽ cho người dùng xem danh sách các file (hoặc đường dẫn nếu file đó nằm trong thư mục con).
### Tính năng Download
Khi nhập 2 sẽ thực hiện tính năng dowload, chương trình sẽ cho người dùng xem danh sách các file (hoặc đường dẫn file) của server. Khi đó người dùng phải nhập chính xác tên file (hoặc đường dẫn file) muốn tải về được hiển thị.
### Tính năng Upload
Khi nhập 3 sẽ thực hiện tính năng upload, ban đầu chương trình sẽ yêu cầu nhập mật khẩu để gửi yêu cầu xác thực đến server. Nếu mật khẩu nhập đúng thì chương trình sẽ hiện ra danh sách các file của client, người dùng muốn upload file nào cần nhập đúng tên file đó.
