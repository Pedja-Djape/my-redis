# Chapter 4: Application Protocol

## 4.1 Multiple Requests in a single connection

```
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
        }

        do_something(connfd);
        close(connfd);
    }
```

With the code above we'll only be able to process one request at a time as it reads the request and writes the response.
However, how does the server know how many bytes to read? This is the role of the application protocol. 

Typically, a protocol has 2 main structures:
1. High-level structure to split up the bytestream received into `n` separate messages.
2. The structure within a message --> deserialization.

### Simple Binary Protocol
1. split the byte steam into different messages
Request/reponses are just strings. The client sends variable length string and server responds with the same protocol.

Each msg consists of a 4byte little-endian integer indicating the length of the request and the variable length payload.
**This is not the redis protocol.**

## 4.2 Parse the protocol

