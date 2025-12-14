/*******************************************************
 * Minimal FTP Client (Assignment Version - Option A)
 * Implements:
 *   - URL parsing (ftp://[user:pass@]host/path)
 *   - DNS resolution (getaddrinfo)
 *   - FTP control connection
 *   - USER/PASS
 *   - TYPE I
 *   - PASV
 *   - Data connection
 *   - RETR
 *   - File download
 *   - QUIT
 *
 * Compile:
 *   gcc ftp_client.c -o ftp_client
 *
 * Usage:
 *   ./ftp_call ftp://[user:pass@]hostname/path/to/file
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

/*********************** Utility ************************/

// Receive a full FTP server line reply into buffer.
// Returns the 3-digit FTP reply code (integer).
int ftp_read_response(int sock, char *buf, size_t max)
{
    memset(buf, 0, max);
    int n = read(sock, buf, max-1);
    if (n <= 0) {
        fprintf(stderr, "Error: failed to read server response\n");
        return -1;
    }
    // FTP lines start with a 3-digit code
    return atoi(buf);
}

// Send a formatted FTP command and flush with CRLF
int ftp_send_cmd(int sock, const char *fmt, const char *arg)
{
    char cmd[256];
    if (arg)
        snprintf(cmd, sizeof(cmd), fmt, arg);
    else
        snprintf(cmd, sizeof(cmd), "%s", fmt);

    // Ensure CRLF
    strcat(cmd, "\r\n");

    if (write(sock, cmd, strlen(cmd)) < 0) {
        perror("write");
        return -1;
    }
    return 0;
}

/*********************** URL Parsing ********************/

struct url_info {
    char user[128];
    char pass[128];
    char host[256];
    char path[512];
    char file[256];
};

int parse_url(const char *url, struct url_info *info)
{
    memset(info, 0, sizeof(*info));

    if (strncmp(url, "ftp://", 6) != 0) {
        fprintf(stderr, "Error: URL must begin with ftp://\n");
        return -1;
    }

    const char *p = url + 6; // skip "ftp://"

    // Check for user:pass@
    const char *at = strchr(p, '@');
    const char *slash = strchr(p, '/');
    if (!slash) {
        fprintf(stderr, "Error: URL missing path component\n");
        return -1;
    }

    if (at && at < slash) {
        // user:pass@
        const char *colon = strchr(p, ':');
        if (!colon || colon > at) {
            fprintf(stderr, "Error: invalid user:pass in URL\n");
            return -1;
        }
        strncpy(info->user, p, colon - p);
        strncpy(info->pass, colon + 1, at - colon - 1);
        p = at + 1; // host begins after '@'
    } else {
        // anonymous login
        strcpy(info->user, "anonymous");
        strcpy(info->pass, "anonymous@example.com");
    }

    // Host is between p and slash
    strncpy(info->host, p, slash - p);

    // Path is everything after slash
    strcpy(info->path, slash + 1);

    // Extract filename from path
    const char *last_slash = strrchr(info->path, '/');
    if (last_slash)
        strcpy(info->file, last_slash + 1);
    else
        strcpy(info->file, info->path);

    if (info->file[0] == '\0') {
        fprintf(stderr, "Error: could not determine filename from URL\n");
        return -1;
    }

    return 0;
}

/*********************** PASV Parsing ********************/

// Parse PASV response (227 Entering Passive Mode (h1,h2,h3,h4,p1,p2))
int parse_pasv(const char *response, char *ip, int *port)
{
    const char *start = strchr(response, '(');
    const char *end = strchr(response, ')');
    if (!start || !end) return -1;

    int h1,h2,h3,h4,p1,p2;
    if (sscanf(start+1, "%d,%d,%d,%d,%d,%d",
               &h1,&h2,&h3,&h4,&p1,&p2) != 6)
        return -1;

    sprintf(ip, "%d.%d.%d.%d", h1,h2,h3,h4);
    *port = p1*256 + p2;
    return 0;
}

/*********************** Main Logic **********************/

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[user:pass@]host/path\n", argv[0]);
        return 1;
    }

    struct url_info url;
    if (parse_url(argv[1], &url) < 0)
        return 1;

    printf("Host: %s\nUser: %s\nFile: %s\nPath: %s\n",
           url.host, url.user, url.file, url.path);

    /*************** DNS Resolution ***************/
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(url.host, "21", &hints, &res) != 0) {
        fprintf(stderr, "Error: DNS resolution failed\n");
        return 1;
    }

    /*************** Control Connection ***************/
    int ctrl_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (ctrl_sock < 0) {
        perror("socket");
        return 1;
    }
    if (connect(ctrl_sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        return 1;
    }
    freeaddrinfo(res);

    char buf[BUFFER_SIZE];
    int code = ftp_read_response(ctrl_sock, buf, sizeof(buf));
    if (code != 220) {
        fprintf(stderr, "Error: server did not send 220 greeting\n");
        return 1;
    }

    /*************** Login ***************/
    ftp_send_cmd(ctrl_sock, "USER %s", url.user);
    code = ftp_read_response(ctrl_sock, buf, sizeof(buf));
    if (code == 331) {
        ftp_send_cmd(ctrl_sock, "PASS %s", url.pass);
        code = ftp_read_response(ctrl_sock, buf, sizeof(buf));
    }
    if (code != 230) {
        fprintf(stderr, "Error: login failed (code %d)\n", code);
        return 1;
    }

    /*************** Binary Mode ***************/
    ftp_send_cmd(ctrl_sock, "TYPE I", NULL);
    code = ftp_read_response(ctrl_sock, buf, sizeof(buf));
    if (code != 200) {
        fprintf(stderr, "Error: TYPE I failed\n");
        return 1;
    }

    /*************** PASV Mode ***************/
    ftp_send_cmd(ctrl_sock, "PASV", NULL);
    code = ftp_read_response(ctrl_sock, buf, sizeof(buf));
    if (code != 227) {
        fprintf(stderr, "Error: PASV failed\n");
        return 1;
    }

    char data_ip[64];
    int data_port;
    if (parse_pasv(buf, data_ip, &data_port) < 0) {
        fprintf(stderr, "Error: could not parse PASV response\n");
        return 1;
    }

    /*************** Data Connection ***************/
    printf("Connecting data socket to %s:%d\n", data_ip, data_port);
    struct addrinfo hints2, *res2;
    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family   = AF_INET;
    hints2.ai_socktype = SOCK_STREAM;

    char portstr[16];
    sprintf(portstr, "%d", data_port);

    if (getaddrinfo(data_ip, portstr, &hints2, &res2) != 0) {
        fprintf(stderr, "Error: resolving PASV IP/port\n");
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

    /*************** RETR Command ***************/
    ftp_send_cmd(ctrl_sock, "RETR %s", url.path);
    code = ftp_read_response(ctrl_sock, buf, sizeof(buf));
    if (code != 150 && code != 125) {
        fprintf(stderr, "Error: RETR failed (code %d)\n", code);
        return 1;
    }

    /*************** Download File ***************/
    FILE *fp = fopen(url.file, "wb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    int n;
    while ((n = read(data_sock, buf, sizeof(buf))) > 0)
        fwrite(buf, 1, n, fp);

    fclose(fp);
    close(data_sock);

    // Read final 226 message
    ftp_read_response(ctrl_sock, buf, sizeof(buf));

    /*************** QUIT ***************/
    ftp_send_cmd(ctrl_sock, "QUIT", NULL);
    ftp_read_response(ctrl_sock, buf, sizeof(buf));
    close(ctrl_sock);

    printf("Download completed: %s\n", url.file);
    return 0;
}
