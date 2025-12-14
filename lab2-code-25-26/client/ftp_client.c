/*
 * Minimal FTP Client
 * Implements:
 *   - URL parsing (ftp://[user:pass@]host/path)
 *   - DNS resolution
 *   - FTP control connection
 *   - Username/Password or Anonymous login
 *   - TYPE I (binary mode)
 *   - Passive mode
 *   - Data connection
 *   - RETR
 *   - File download
 *   - QUIT
 *
 * Compile:
 *   gcc ftp_client.c -o ftp_client
 *
 * Usage:
 *   ./ftp_client ftp://[user:pass@]hostname/path
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

typedef struct {
    char user[128];
    char pass[128];
    char host[256];
    char path[512];
    char file[256];
} url_info;

// Utility ----------------------------------------------------------------

// Receive a full FTP server line reply into buffer.
// Returns the 3-digit FTP reply code (integer).
int read_reply(int sock, char *buffer, size_t max)
{
    memset(buffer, 0, max);
    int bytes = read(sock, buffer, max-1);
    if (bytes <= 0) {
        fprintf(stderr, "Error: failed to read server response\n");
        return EXIT_FAILURE;
    }
    // FTP lines start with a 3-digit code
    return atoi(buffer);
}

// Send a formatted FTP command and flush with CRLF
int send_cmd(int sock, const char *fmt, const char *arg)
{
    char cmd[256];
    if (arg)
        snprintf(cmd, sizeof(cmd), fmt, arg);
    else
        snprintf(cmd, sizeof(cmd), "%s", fmt);

    // ensure CRLF
    strcat(cmd, "\r\n");

    if (write(sock, cmd, strlen(cmd)) < 0) {
        perror("write");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// URL Parsing -----------------------------------------------------------

int parse_url(const char *url, url_info *info)
{
    memset(info, 0, sizeof(*info));

    if (strncmp(url, "ftp://", 6) != 0) {
        fprintf(stderr, "Error: URL must begin with ftp://\n");
        return EXIT_FAILURE;
    }

    const char *p = url + 6; // skip "ftp://"

    // check for user:pass@
    const char *at = strchr(p, '@');
    const char *slash = strchr(p, '/');
    if (!slash) {
        fprintf(stderr, "Error: URL missing path\n");
        return EXIT_FAILURE;
    }

    if (at && at < slash) {
        // user:pass@
        const char *colon = strchr(p, ':');
        if (!colon || colon > at) {
            fprintf(stderr, "Error: invalid user:pass in URL\n");
            return EXIT_FAILURE;
        }
        strncpy(info->user, p, colon - p);
        strncpy(info->pass, colon + 1, at - colon - 1);
        p = at + 1; // host begins after '@'
    } else {
        // anonymous login
        strcpy(info->user, "anonymous");
        strcpy(info->pass, "anonymous@example.com");
    }

    // host is between p and slash
    strncpy(info->host, p, slash - p);

    // path is everything after slash
    strcpy(info->path, slash + 1);

    // extract filename from path
    const char *last_slash = strrchr(info->path, '/');
    if (last_slash)
        strcpy(info->file, last_slash + 1);
    else
        strcpy(info->file, info->path);

    if (info->file[0] == '\0') {
        fprintf(stderr, "Error: could not determine filename from URL\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Passive Mode Parsing ------------------------------------------------

int parse_passive(const char *response, char *ip, int *port)
{
    const char *start = strchr(response, '(');
    const char *end = strchr(response, ')');
    if (!start || !end) return EXIT_FAILURE;

    int h1,h2,h3,h4,p1,p2;
    if (sscanf(start+1, "%d,%d,%d,%d,%d,%d",
               &h1,&h2,&h3,&h4,&p1,&p2) != 6)
        return EXIT_FAILURE;

    sprintf(ip, "%d.%d.%d.%d", h1,h2,h3,h4);
    *port = p1*256 + p2;
    return EXIT_SUCCESS;
}

// Main: parse and download -------------------------------------------------

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[user:pass@]host/path\n", argv[0]);
        return 1;
    }

    url_info url;
    if (parse_url(argv[1], &url) < 0)
        return 1;

    printf("Host: %s\nUser: %s\nFile: %s\nPath: %s\n", url.host, url.user, url.file, url.path);

    // DNS resolution using gethostbyname
    struct hostent *server;
    server = gethostbyname(url.host);
    if (server == NULL) {
        fprintf(stderr, "Error: unknown host %s\n", url.host);
        return 1;
    }

    // setup control socket address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(21);
    memcpy(&server_addr.sin_addr.s_addr,
        server->h_addr,
        server->h_length);

    // create control socket
    int ctrl_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctrl_sock < 0) {
        perror("socket");
        return 1;
    }

    if (connect(ctrl_sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    char buf[BUFFER_SIZE];
    int code = read_reply(ctrl_sock, buf, sizeof(buf));
    if (code != 220) {
        fprintf(stderr, "Error: server did not send 220 greeting\n");
        return 1;
    }

    printf("Control socket connected.\n");

    // login
    send_cmd(ctrl_sock, "USER %s", url.user);
    code = read_reply(ctrl_sock, buf, sizeof(buf));
    if (code == 331) {
        send_cmd(ctrl_sock, "PASS %s", url.pass);
        code = read_reply(ctrl_sock, buf, sizeof(buf));
    }
    if (code != 230) {
        fprintf(stderr, "Error: login failed (code %d)\n", code);
        return 1;
    }
    printf("Login successful.\n");

    // binary mode set
    send_cmd(ctrl_sock, "TYPE I", NULL);
    code = read_reply(ctrl_sock, buf, sizeof(buf));
    if (code != 200) {
        fprintf(stderr, "Error: TYPE I failed\n");
        return 1;
    }
    printf("Binary mode set.\n");

    // passive mode set
    send_cmd(ctrl_sock, "PASV", NULL);
    code = read_reply(ctrl_sock, buf, sizeof(buf));
    if (code != 227) {
        fprintf(stderr, "Error: PASV failed\n");
        return 1;
    }

    char data_ip[64];
    int data_port;
    if (parse_passive(buf, data_ip, &data_port) < 0) {
        fprintf(stderr, "Error: could not parse PASV response\n");
        return 1;
    }
    printf("Passive mode set.\n");

    // data socket set
    printf("Connecting data socket to %s:%d\n", data_ip, data_port);
    struct addrinfo hints2, *res2;
    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family = AF_INET;
    hints2.ai_socktype = SOCK_STREAM;

    char portstr[16];
    sprintf(portstr, "%d", data_port);

    if (getaddrinfo(data_ip, portstr, &hints2, &res2) != 0) {
        fprintf(stderr, "Error: resolving passive IP/port\n");
        return 1;
    }

    int data_sock = socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol);
    if (data_sock < 0) {
        perror("socket");
        return 1;
    }
    if (connect(data_sock, res2->ai_addr, res2->ai_addrlen) < 0) {
        perror("connect");
        return 1;
    }

    freeaddrinfo(res2);

    printf("Data socket connected.\n");

    // RETR command
    send_cmd(ctrl_sock, "RETR %s", url.path);
    code = read_reply(ctrl_sock, buf, sizeof(buf));
    if (code != 150 && code != 125) {
        fprintf(stderr, "Error: RETR failed (code %d)\n", code);
        return 1;
    }

    // download file
    FILE *fp = fopen(url.file, "wb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    int n;
    while ((n = read(data_sock, buf, sizeof(buf))) > 0){
        fwrite(buf, 1, n, fp);
    }

    fclose(fp);
    close(data_sock);

    // read final 226 message
    read_reply(ctrl_sock, buf, sizeof(buf));

    // stop process (QUIT)
    send_cmd(ctrl_sock, "QUIT", NULL);
    read_reply(ctrl_sock, buf, sizeof(buf));
    close(ctrl_sock);

    printf("Download completed: %s\n", url.file);
    return EXIT_SUCCESS;
}
