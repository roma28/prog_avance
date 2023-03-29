## Client usage:

Client gets its configuration from the file named ```client_config``` with the following syntax:

* First line defines how many servers will be connected (N)
* N following lines set up client parameters as ``` [ip addres] [port] [filename] ```

Example:
```
2
127.0.0.1 8080 1.jpg
127.0.0.1 8081 2.jpg
```

Client connects to the servers and updates the files as they updated in the server side  
Client also creates a self-updating HTML page ```main.html``` which shows the images

## Server usage
Server is run as a console application with 2 arguments as ```./server [port] [filename]```

After a connection is received from a client server starts to 
monitor updates in the file and transmits it to the client.