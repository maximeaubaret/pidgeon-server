#ifndef CLIENT_H_
#define CLIENT_H_

typedef struct {
  int     fd;           /* socket descriptor */
  int     uid;          /* user unique identifier */
  char *  nickname;     /* user nickname */

  int     state;

  char    tx_buff[200];
  char    rx_buff[200];

  pthread_mutex_t m_tx_buff;

  struct sockaddr_in  sockaddr;
  pthread_t    thread;
} Client;

enum {
  CLIENT_STATE_WELCOME,
  CLIENT_STATE_WAITING,
  CLIENT_STATE_PARSE,
  CLIENT_STATE_BROADCAST,
  CLIENT_STATE_UNICAST,
  CLIENT_STATE_USAGE,
  CLIENT_STATE_NICKNAME,
  CLIENT_STATE_ME,
  CLIENT_STATE_LIST,
  CLIENT_STATE_END,
  CLIENT_STATE
};

Client *client_new ();
void    client_free (Client *c);

#endif
