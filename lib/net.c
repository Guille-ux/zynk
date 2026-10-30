#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <sys/time.h>
#include <netdb.h>
#include "../include/eval.h"
#include "../include/globals.h"

int newSocket() {
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	
	return socket_fd;
}

int Connect(int sock, const char *ip, int port) {
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &serv_addr.sin_addr);

	return connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
}

int Send(int sock, char *str) {
	return send(sock, str, strlen(str), 0);
}

int Recv(int sock, char *buffer, int size, bool nblock) {
	return recv(sock, buffer, size, nblock ? MSG_DONTWAIT : 0);
}

int Bind(int sock, int port) {
	struct sockaddr_in serv;
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port);
	serv.sin_addr.s_addr = INADDR_ANY;
	return bind(sock, (struct sockaddr *)&serv, sizeof(serv));
}

int Listen(int sock, int many) {
	return listen(sock, many);
}

int Accept(int sock) {
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

	return accept(sock, (struct sockaddr *)&address, &addrlen);
}

int Close(int sock) {
	return close(sock);
}


ShellValue native_getip(int argc, ShellValue *args) {
	if (argc < 1 || args[0].type != VAL_STRING) return newNumVal(-1);
	char *dominio = args[0].as.str;
	
	struct hostent *he = gethostbyname(dominio);
	if (he == NULL) {
		herror("Error resolviendo el host");
		return newNumVal(-1);
	}

	struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
	
	if (addr_list[0] != NULL) {
		char ip_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, addr_list[0], ip_str, sizeof(ip_str));
		return newStrVal(ip_str);
	}
	return newNumVal(-1);
}

ShellValue native_socket(int argc, ShellValue *args) {
	return newNumVal((double)newSocket());
}

ShellValue native_connect(int argc, ShellValue *args) {
	if (argc < 3) return newNumVal(-1);
	int sock = (int)args[0].as.num;
	const char *ip = args[1].as.str;
	int port = (int)args[2].as.num;

	return newNumVal((double)Connect(sock, ip, port));
}

ShellValue native_send(int argc, ShellValue *args) {
	if (argc < 2) return newNumVal(-1);
	int sock = (int)args[0].as.num;
	char *msg = args[1].as.str;
	return newNumVal((double)Send(sock, msg));
}

#define BUFFER_SIZE 1024
static char s_buffer[BUFFER_SIZE];

ShellValue native_recv(int argc, ShellValue *args) {
	if (argc < 2) return newStrVal("");
	int sock = (int)args[0].as.num;
	bool nblock = ((int)args[1].as.num == 0) ? true : false;
	memset(s_buffer, 0, BUFFER_SIZE);

	int bytes = Recv(sock, s_buffer, BUFFER_SIZE - 1, nblock);
	s_buffer[BUFFER_SIZE - 1] = '\0';
	if (bytes < 0) return newStrVal("");
	return newStrVal(s_buffer);
}

ShellValue native_bind(int argc, ShellValue *args) {
	if (argc < 2) return newNumVal(-1);
	int sock = (int)args[0].as.num;
	int port = (int)args[1].as.num;

	return newNumVal((double)Bind(sock, port));
}

ShellValue native_listen(int argc, ShellValue *args) {
	if (argc < 2) return newNumVal(-1);
	int sock = (int)args[0].as.num;
	int num = (int)args[1].as.num;

	return newNumVal((double)Listen(sock, num));
}

ShellValue native_accept(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	int sock = (int)args[0].as.num;

	return newNumVal((double)Accept(sock));
}

ShellValue native_close(int argc, ShellValue *args) {
	if (argc < 1) return newNumVal(-1);
	int sock = (int)args[0].as.num;

	return newNumVal((double)Close(sock));
}


void register_lib(EvalCtx *ctx) {
	EvalCtx *net_ctx = newShellCtx();
	
	register_cfunc(net_ctx, "getip", native_getip);
	register_cfunc(net_ctx, "socket", native_socket);
	register_cfunc(net_ctx, "connect", native_connect);
	register_cfunc(net_ctx, "send", native_send);
	register_cfunc(net_ctx, "recv", native_recv);
	register_cfunc(net_ctx, "bind", native_bind);
	register_cfunc(net_ctx, "listen", native_listen);
	register_cfunc(net_ctx, "accept", native_accept);
	register_cfunc(net_ctx, "close", native_close);

	
	register_class(ctx, "Net", net_ctx);
}
