#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

void die(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    int sock, conn_sock, port, bytes_read;
    socklen_t client_addr_len;
    char buffer[1024];
    struct sockaddr_in server_addr, client_addr;

    // Check for arguments
    if (argc < 2)
    {
        die("Usage: ./server [port]");
    }

    // create TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        die("Error opening socket");
    }

    // Get port from command line arguments, convert to integer.
    errno = 0;
    port = strtol(argv[1], NULL, 10);
    if (errno == EINVAL || errno == ERANGE)
    {
        die("Invalid port");
    }

    // Initialize socket address
    bzero((char*) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket
    if (bind(sock, (struct sockaddr*) &server_addr, sizeof(server_addr)))
    {
        die("Error binding socket address");
    }

    // Start listening
    listen(sock, 10);

    for (;;)
    {
        // Accept connection
        conn_sock = accept(sock, (struct sockaddr*) &client_addr, &client_addr_len);
        if (conn_sock < 0)
        {
            die("Error accepting connection");
        }

        // Read request from client
        bytes_read = read(conn_sock, buffer, sizeof(buffer));
        if (bytes_read < 0)
        {
            die("Error reading from connection");
        }
        buffer[bytes_read] = '\0';

        // Parse HTTP request
        // Get first line
        char* request_line = strtok(buffer, "\n");
        if (request_line == NULL)
        {
            die("Invalid HTTP request");
        }
        // Get requested file
        strtok(request_line, " "); // throw away first token
        char* requested_file = strtok(NULL, " ");
        if (requested_file == NULL)
        {
            die("Invalid HTTP request format");
        }

        // Read requested file from www directory
        char* filename;
        if (strcmp(requested_file, "/") == 0)
        {
            // Read index file
            filename = (char*)malloc(15);
            strcpy(filename, "www/index.html");
            filename[14] = '\0';
        }
        else
        {
            // Read requested file
            int filename_len = strlen(requested_file) + 4; // +4 for "www" and '\0'
            filename = (char*)malloc(filename_len);
            strcpy(filename, "www/");
            strcpy(filename + 3, requested_file);
            filename[filename_len - 1] = '\0';
        }
        // Open the file
        printf("GET %s - ", filename);
        FILE* fp = fopen(filename, "r");
        free(filename);
        if (fp == NULL)
        {
            // TODO: 404 response instead
            char* response = "HTTP/1.0 404 Not Found\r\n\r\n";
            printf("HTTP/1.0 404 Not Found\n");
            int response_size = strlen(response);
            int bytes_written = 0;
            do {
                int num_bytes = write(conn_sock, response + bytes_written, response_size - bytes_written);
                if (num_bytes == -1)
                {
                    die("Error writing to connection");
                }
                bytes_written += num_bytes;
            } while (bytes_written < response_size);
        }
        else
        {
            // Get file size
            fseek(fp, 0L, SEEK_END);
            int file_size = ftell(fp);
            rewind(fp);

            // Allocate buffer and read file contents
            const char* header = "HTTP/1.0 200 OK\r\n\r\n";
            printf("HTTP/1.0 200 OK\n");
            int response_size = file_size + strlen(header) + 1;
            char* file_buffer = (char*) malloc(response_size); // +1 for '\0'
            strcpy(file_buffer, header);
            bytes_read = fread(file_buffer + strlen(header), 1, file_size, fp);
            if (bytes_read < file_size)
            {
                die("Error reading file");
            }
            file_buffer[response_size - 1] = '\0';

            // Respond to client
            int bytes_written = 0;
            do {
                int num_bytes = write(conn_sock, file_buffer + bytes_written, response_size - bytes_written);
                if (num_bytes == -1)
                {
                    die("Error writing to connection");
                }
                bytes_written += num_bytes;
            } while (bytes_written < response_size);

            free(file_buffer);
        }

        close(conn_sock);
    }


    close(sock);
    exit(EXIT_SUCCESS);
}
