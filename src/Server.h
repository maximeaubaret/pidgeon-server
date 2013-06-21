#ifndef SERVER_H_
#define SERVER_H_

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#include "List.h"
#include "Client.h"

typedef struct {
  int fd;
  int port;
  int running;

  List *users;
  pthread_mutex_t m_users;

  struct sockaddr_in  sockaddr;
} Server;

Server *  server_new ();
void      server_listen (Server *s);
void      server_free (Server *s);

void      server_set_port (Server *s, int port);
int       server_get_port (Server *s);

#endif

