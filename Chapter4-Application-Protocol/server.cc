#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <assert.h>
#include <iostream>

// max msg size
const size_t k_max_msg = 4096;


static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// n is the size of the data to read
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        // rv is the size of the message read in
        ssize_t rv = read(fd, buf, n);
            if (rv <= 0) {
                return -1; // error or unnexpected EOF
            }
        assert((size_t) rv <= n);
        // update payload size
        n -= (size_t) rv;
        // increase the pointer to point to the start of the next part of the payload
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1; // error
        }
        assert((size_t) rv <= n);
        n -= (size_t) rv;
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd) {
    // 4bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }
    uint32_t len = 0;
    // copy 4 bytes from rbuf to len (i.e., get the size of the message)
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        msg("err: msg too long");
        return -1;
    }
    // request body
    // start reading message at the point after the size is specified
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    
    // do something
    // add EOF to string
    rbuf[4 + len] = '\0';
    std::cout << "Client says " << &rbuf[4] << std::endl; 
    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t) strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, len + 4);
}

int main() {
    // doesn't create the socket just yet
    // first param is for ipv4 and sockstream is for TCP
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1; 
    // setting socket options
    /*
        The effect of SO_REUSEADDR is important: if it’s not set to 1,
        a server program cannot bind to the same IP:port it was using
        after a restart.
    */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    // this creates a mapping between the socket and an address on the network
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN); // listen on this address for incoming connections
    if (rv) {
        die("listen()");
    }

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        // accept returns the peers address
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }

        close(connfd);
    }

    return 0;
}
