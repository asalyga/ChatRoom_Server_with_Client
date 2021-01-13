#include "client.h"
#include <string>
/*****************************************/
/* 			TCP CLIENT CLASS			 */
/*****************************************/

TCP_Client::TCP_Client()
{
    fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd == -1) {
        perror("socket");
    }
}

TCP_Client::~TCP_Client(void)
{
    close(fd);
}

int TCP_Client::get_fd(void)
{
    return fd;
}
// Connect to a server
int TCP_Client::connect_t(const char* server_addr,
                          uint16_t port_no,
                          timeout_sec tout, timeout_sec retry)
{
    assert(server_addr != nullptr);

    sockaddr_in caddr;
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(port_no);
    caddr.sin_addr.s_addr = inet_addr(server_addr);
    socklen_t len = sizeof(struct sockaddr_in);

    float t_elapsed;

    timespec current, prev;
    clock_gettime( CLOCK_REALTIME, &prev );
    do {
        if(!connect (fd, reinterpret_cast<sockaddr*>(&caddr), len))
            return COMM_SUCCESS;

        sleep(retry);
        clock_gettime( CLOCK_REALTIME, &current);
        t_elapsed = ((current.tv_sec - prev.tv_sec)
                     + static_cast<float>(current.tv_nsec - prev.tv_nsec)/1000000000.0f);
    } while(t_elapsed < tout);

    perror("connect");

    return COMM_FAILURE;
}

// HandShaking
int TCP_Client::HandleConn()
{
    int bytes_expected = 0;
    int ret = 1;


    cout << "Enter User Name:" << endl;
    char s1[256];
    cin.getline(s1, sizeof(s1));
    u_int32_t bytes_to_send = strlen(s1);
    cwrite(&bytes_to_send, sizeof(bytes_to_send));
    cwrite(s1, bytes_to_send);

    ret = ccread_t(&bytes_expected, sizeof(bytes_expected), 60);
    if(ret > 0) {
        char rx_buf[bytes_expected + 1];
        ret = ccread_t(rx_buf, bytes_expected, 60);
        if(ret > 0) {
            rx_buf[bytes_expected] = '\0';
            cout << "Received Message:\n" << rx_buf << endl;
        }
    }

    while(ret)
    {
        int in;
        cout << "Press 1- Send Text\n2-Send File" << endl;
        cin >> in;
        cin.get();
        if(in == 1) {
            cout << "Enter Sender Message:" << endl;
            char s[255], s_temp[256];
            cin.getline(s, sizeof(s));

            s_temp[0] = in;
            memcpy(&s_temp[1], s, strlen(s));
            u_int32_t bytes_to_send = strlen(s_temp);
            cwrite(&bytes_to_send, sizeof(bytes_to_send));
            cwrite(s_temp, bytes_to_send);

            ret = ccread_t(&bytes_expected, sizeof(bytes_expected), 60);
            if(ret > 0) {
                char rx_buf[bytes_expected + 1];
                ret = ccread_t(rx_buf, bytes_expected, 60);
                if(ret > 0) {
                    rx_buf[bytes_expected] = '\0';
                    cout << "Received Message:\n" << rx_buf << endl;
                }
            }
            else
                break;
        }
        else if(in == 2) {
            cout << "Enter Complete File Path:" << endl;
            char s[255], s_temp[256];
            cin.getline(s, sizeof(s));

            s_temp[0] = in;
            memcpy(&s_temp[1], s, strlen(s));
            u_int32_t bytes_to_send = strlen(s_temp);
            cwrite(&bytes_to_send, sizeof(bytes_to_send));
            cwrite(s_temp, bytes_to_send);

            sendCustomFile(s);

            ret = ccread_t(&bytes_expected, sizeof(bytes_expected), 60);
            if(ret > 0) {
                char rx_buf[bytes_expected + 1];
                ret = ccread_t(rx_buf, bytes_expected, 60);
                if(ret > 0) {
                    rx_buf[bytes_expected] = '\0';
                    cout << "Received Message:\n" << rx_buf << endl;
                }
            }
            else
                break;

        }
    }

    return 0;
}

ssize_t TCP_Client::read_t(int fd, void *buf, size_t count, timeout_sec t)
{
    assert(fd > 0);

    timeval t_sec;
    fd_set readfds, writefds, exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    t_sec.tv_sec = t;
    t_sec.tv_usec = 0;
    FD_SET(fd, &readfds);

    // checks if there's something available to read within in timeout value
    if(select(fd+1, &readfds, &writefds, &exceptfds, &t_sec) < 0) {
        perror("select");
        return COMM_FAILURE;
    }
    else if (FD_ISSET(fd, &readfds))
        return read(fd, buf, count);
    else
        return COMM_FAILURE;
}

ssize_t TCP_Client::cread_t(int fd, void *dst, size_t n, timeout_sec t)
{
    assert(dst != nullptr);
    assert(fd > 0);

    ssize_t bytes_recv = 0;
    size_t bytes_left = n;
    uint8_t *dst_buf = static_cast<uint8_t *> (dst);

    while(bytes_left) {
        bytes_recv = read_t(fd, dst_buf, bytes_left, t);
        if(bytes_recv <= 0) {
            perror("read");
            return COMM_FAILURE;
        }
        assert(static_cast<ssize_t>(bytes_left) >= bytes_recv);
        bytes_left -= static_cast<size_t>(bytes_recv);
        dst_buf += bytes_recv;
    }
    return static_cast<ssize_t>(n);
}

// client complete read with timeout
ssize_t TCP_Client::ccread_t(void *buf, size_t n, timeout_sec t)
{
    assert(buf != nullptr);

    int cfd = get_fd();
    if(cfd > 0)
        return cread_t(cfd, buf, n, t);
    else
        return COMM_FAILURE;
}

// client write
ssize_t TCP_Client::cwrite(void *buf, size_t n)
{
    assert(buf != nullptr);

    int cfd = get_fd();
    if(cfd > 0)
        return write(cfd, buf, n);
    else
        return COMM_FAILURE;
}

ssize_t TCP_Client::completeSend(int fd_in, size_t n)
{
    int fd = get_fd();
    if(fd <= 0)
        return COMM_FAILURE;

    else {
        ssize_t bytes_sent = 0;
        size_t bytes_left = n;
        long int offset = 0;

        while(bytes_left > 0) {
            bytes_sent = sendfile(fd, fd_in, &offset, bytes_left);
            if(bytes_sent <= 0) {
                perror("sendfile");
                return -1;
            }
            assert((ssize_t)(bytes_left) >= bytes_sent);
            bytes_left -= (ssize_t)(bytes_sent);
        }
        return (ssize_t)(n);
    }
}

ssize_t TCP_Client::recvFile(const char *filename, size_t n, u_int16_t t)
{
    assert(filename != NULL);

    int fd = get_fd();
    if(fd <= 0)
        return COMM_FAILURE;

    else {
        char buffer[n];

        FILE *received_file = fopen(filename, "w");
        if (received_file == NULL)
        {
            perror("fopen");
            return COMM_FAILURE;
        }

        ccread_t(buffer, n, t);
        fwrite(buffer, sizeof(char), n, received_file);
        fclose(received_file);

        return n;
    }
}

ssize_t TCP_Client::sendCustomFile(const char *filename)
{
    assert(filename != NULL);

    int fd = get_fd();
    if(fd <= 0)
        return COMM_FAILURE;

    else {

        int file_to_send = open(filename, O_RDONLY);
        if (file_to_send == 0)
        {
            perror("fopen");
            return COMM_FAILURE;
        }

        // Get file size
        struct stat my_file_stat;
        if (fstat(file_to_send, &my_file_stat) < 0)
        {
            perror("fstat");
            return COMM_FAILURE;
        }

        u_int32_t file_size = my_file_stat.st_size;
        write(fd, &file_size, sizeof(file_size));
        completeSend(file_to_send, file_size);

        close(file_to_send);

        return file_size;
    }
}

void ClientTest() {
    TCP_Client client;
    cout << "Trying to connect to client" << endl;
    client.connect_t("127.0.0.1", 6000, 10, 10);
    cout << "Found Server" << endl;
    client.HandleConn();
}
