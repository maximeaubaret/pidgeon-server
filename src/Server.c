#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>

#include "Utils.h"
#include "Server.h"
#include "Client.h"


void  _server_wait_for_client (Server *s);

void *_server_client_loop (void *p);

int   _server_welcome (Server *s, Client *c);
void  _server_send_usage (Server *s, Client *c);

void  _server_send_broadcast_from_server (Server *s, char *msg);
void  _server_send_msg_from_server (Server *s, Client *c, char *msg);
void  _server_send_msg (Server *s, Client *from, Client *to, char *msg);
void  _server_send_msg_me (Server *s, Client *from, Client *to, char *msg);
void  _server_send_private_msg (Server *s, Client *from, Client *to, char *msg);

int   _server_recv_msg (Server *s, Client *c);
int   _server_parse_msg (Server *s, Client *c);

int   _server_unicast_send (Server *s, Client *c);
int   _server_broadcast_send (Server *s, Client *c);
int   _server_broadcast_send_me (Server *s, Client *c);

int   _server_parse_nickname (Server *s, Client *c);

int   _server_send_list (Server *s, Client *c);

void  _server_log (char *msg);



/**
 * server_new:
 *
 * Allocates space for one #Server.
 *
 * Returns: a pointer to the newly-allocated #Server
 */
Server *
server_new ()
{
  return calloc (1, sizeof (Server));
}

/**
 * server_listen:
 * @server: a #Server
 *
 * Set a #Server in listen mode.
 *
 * Returns: a pointer to the newly-allocated #Server
 */
void
server_listen (
    Server *s)
{
  if (s != NULL) {
    s->fd = socket (PF_INET, SOCK_STREAM, 0);
    if (s->fd == -1)
      die ("Could not create the socket");

    memset (&(s->sockaddr), '\0', sizeof (struct sockaddr_in));

    s->sockaddr.sin_family = AF_INET;
    s->sockaddr.sin_addr.s_addr = htons (INADDR_ANY);
    s->sockaddr.sin_port = htons (s->port);

    if (bind (s->fd, (struct sockaddr *)&(s->sockaddr), sizeof (struct sockaddr_in)) == -1) 
      die ("Bind failed");

    if (listen (s->fd, 10) == -1)
      die ("Listen failed");

    char *msg;
    asprintf(&msg, "Listening on port %i...", s->port);
    _server_log (msg);
    free(msg);

    /* Mutex */
    pthread_mutex_init (&(s->m_users), NULL);


    /* Go! */
    s->running = 1;

    while (s->running) {
      _server_wait_for_client (s);
    }
  }
}

void
_server_spawn_client (
    Server *s, 
    Client *c)
{
  void **p = calloc (2, sizeof (void *));
  p[0] = (void *)s;
  p[1] = (void *)c;
  pthread_create (&(c->thread), NULL, _server_client_loop, p);
}

void
_server_wait_for_client (
    Server *s)
{
  static int clientUID = 1;
  char nick[200];

  Client *c = client_new ();
  socklen_t sockaddr_len = sizeof (c->sockaddr);

  c->fd = accept (s->fd, (struct sockaddr *)&(c->sockaddr), &sockaddr_len); 
  if (c->fd == -1)
    die ("Accept failed");

  pthread_mutex_init (&(c->m_tx_buff), NULL);

  _server_log ("A new client joined the room...");
  _server_send_broadcast_from_server (s, "New client connected...");

  c->uid = clientUID;
  sprintf(nick, "guest#%03i", clientUID);
  c->nickname = strdup (nick);

  _server_spawn_client (s, c);

  pthread_mutex_lock (&(s->m_users));
  s->users = list_append (s->users, c);
  pthread_mutex_unlock (&(s->m_users));

  clientUID++;
}

void *
_server_client_loop (
    void *p)
{
  Server *s = ((void **)p)[0];
  Client *c = ((void **)p)[1];
  free (p);

  c->state = CLIENT_STATE_WELCOME;

  while (c->state != CLIENT_STATE) {
    switch (c->state) {
      case CLIENT_STATE_WELCOME:
        c->state = _server_welcome (s, c);
        break;

      case CLIENT_STATE_NICKNAME:
        c->state = _server_parse_nickname (s, c);
        break;

      case CLIENT_STATE_USAGE:
        _server_send_usage (s, c);
        c->state = CLIENT_STATE_WAITING;
        break;

      case CLIENT_STATE_WAITING: 
        c->state = _server_recv_msg (s, c);
        break;

      case CLIENT_STATE_PARSE:
        c->state = _server_parse_msg (s, c);
        break;

      case CLIENT_STATE_BROADCAST:
        c->state = _server_broadcast_send(s, c);
        break;

      case CLIENT_STATE_UNICAST:
        c->state = _server_unicast_send (s, c);
        break;

      case CLIENT_STATE_ME:
        c->state = _server_broadcast_send_me (s, c);
        break;

      case CLIENT_STATE_LIST:
        c->state = _server_send_list (s, c);;
        break;

      case CLIENT_STATE_END:
        close (c->fd);

        pthread_mutex_lock (&(s->m_users));
        s->users = list_remove (s->users, c);
        pthread_mutex_unlock (&(s->m_users));

        char *msg;
        asprintf(&msg, "%s left the room.", c->nickname);
        _server_log (msg);
        _server_send_broadcast_from_server (s, msg);
        free(msg);

        c->state = CLIENT_STATE;
        break;
    }
  }
}

int
_server_welcome (
    Server *s,
    Client *c)
{
  _server_send_msg_from_server (s, c, "Welcome to Pidgeon! Type /help for a basic list of commands!");

  return CLIENT_STATE_WAITING;
}

void  
_server_send_broadcast_from_server (
    Server *s, 
    char *msg)
{
  pthread_mutex_lock (&(s->m_users));
  List *l = s->users;
  while (l) {
    Client *dest = (Client *)l->data;
    if (dest->state != CLIENT_STATE_WELCOME && dest->state != CLIENT_STATE_NICKNAME)
      _server_send_msg_from_server (s, dest, msg);

    l = l->next;
  }
  pthread_mutex_unlock (&(s->m_users));
}

int   
_server_send_list (
    Server *s, 
    Client *c)
{
  List *nicknames = NULL;

  pthread_mutex_lock (&(s->m_users));
  {
    List *l = s->users;
    while (l) {
      nicknames = list_append (nicknames, ((Client *)l->data)->nickname);
      l = l->next;
    }
  }
  pthread_mutex_unlock (&(s->m_users));

  List *l = nicknames;
  _server_send_msg_from_server (s, c, "List of users:");
  while (l) {
    _server_send_msg_from_server (s, c, l->data);
    l = l->next;
  }
  list_free (nicknames);

  return CLIENT_STATE_WAITING;
}

void  
_server_send_msg_from_server (
    Server *s, 
    Client *c, 
    char *msg)
{
  ssize_t len = 0;

  time_t timestamp;
  struct tm *t;
  timestamp = time(NULL);
  t = localtime (&timestamp);

  if (c->fd != -1) {
    len = snprintf (c->tx_buff, sizeof (c->tx_buff), "%02i:%02i:%02i [%sServer%s] %s\n", t->tm_hour, t->tm_min, t->tm_sec, CRED, CNRM, msg);
    write (c->fd, c->tx_buff, len);
  }
}

int
_server_broadcast_send (
    Server *s,
    Client *c)
{
  char *msg;
  asprintf(&msg, "[%s] %s", c->nickname, c->rx_buff);
  _server_log (msg);
  free(msg);

  _server_send_msg (s, c, NULL, c->rx_buff);

  return CLIENT_STATE_WAITING;
}

int
_server_broadcast_send_me (
    Server *s,
    Client *c)
{
  char *msg;
  asprintf(&msg, "*%s %s", c->nickname, c->rx_buff + 4);
  _server_log (msg);
  free(msg);

  _server_send_msg_me (s, c, NULL, c->rx_buff + 4);

  return CLIENT_STATE_WAITING;
}

int   
_server_unicast_send (
    Server *s, 
    Client *c)
{
  List *l;
  Client *d = NULL;

  char *nickname, *msg, *tofree;
  tofree = msg = strdup (c->rx_buff + 5);
  nickname = strsep (&msg, " ");
  free (tofree);

  if (msg == NULL) {
    _server_send_msg_from_server (s, c, "No messages entered.");
    return CLIENT_STATE_WAITING;
  }

  /* Finding destination client*/
  pthread_mutex_lock (&(s->m_users));
  l = s->users;

  while (l) {
    d = (Client *)l->data;
    //TODO: lowercase nicknames
    if (strcmp (nickname, d->nickname) == 0) {
      break;
    }

    l = l->next;
  }

  /* client found */
  if (d != NULL) {
    _server_send_private_msg (s, c, d, msg);
  }
  pthread_mutex_unlock (&(s->m_users));

  if (d == NULL) {
    _server_send_msg_from_server (s, c, "No user found.");
  }

  return CLIENT_STATE_WAITING;
}

void
_server_send_private_msg (
    Server *s,
    Client *from,
    Client *to,
    char *msg)
{
  ssize_t len = 0;

  time_t timestamp;
  struct tm *t;
  timestamp = time(NULL);
  t = localtime (&timestamp);

  if (to->fd != -1) {
    pthread_mutex_lock (&(to->m_tx_buff));
    len = snprintf (to->tx_buff, sizeof (to->tx_buff), "%02i:%02i:%02i [%s%s%s -> %s%s%s] %s\n", t->tm_hour, t->tm_min, t->tm_sec, CGRN, from->nickname, CNRM, CGRN, to->nickname, CNRM, msg);
    write (to->fd, to->tx_buff, len);
    pthread_mutex_unlock (&(to->m_tx_buff));
  }
}

void  
_server_send_msg (
    Server *s, 
   Client *from, 
   Client *to, 
    char *msg)
{
  ssize_t len = 0;

  time_t timestamp;
  struct tm *t;
  timestamp = time(NULL);
  t = localtime (&timestamp);

  /* Broadcast to all clients */
  if (to == NULL) {
    pthread_mutex_lock (&(s->m_users));
    List *l = s->users;
    while (l) {
      Client *dest = (Client *)l->data;
      if (dest->state != CLIENT_STATE_WELCOME && dest->state != CLIENT_STATE_NICKNAME)
        _server_send_msg (s, from, dest, msg);
      l = l->next;
    }
    pthread_mutex_unlock (&(s->m_users));
  }
  /* Send only to 'to' */
  else {
    if (to->fd != -1) {
      pthread_mutex_lock (&(to->m_tx_buff));
      len = snprintf (to->tx_buff, sizeof (to->tx_buff), "%02i:%02i:%02i [%s%s%s] %s\n", t->tm_hour, t->tm_min, t->tm_sec, CGRN, from->nickname, CNRM, msg);
      write (to->fd, to->tx_buff, len);
      pthread_mutex_unlock (&(to->m_tx_buff));
    }
  }
}

void  
_server_send_msg_me (
    Server *s, 
   Client *from, 
   Client *to, 
    char *msg)
{
  ssize_t len = 0;

  time_t timestamp;
  struct tm *t;
  timestamp = time(NULL);
  t = localtime (&timestamp);

  /* Broadcast to all clients */
  if (to == NULL) {
    pthread_mutex_lock (&(s->m_users));
    List *l = s->users;
    while (l) {
      Client *dest = (Client *)l->data;
      if (dest->state != CLIENT_STATE_WELCOME && dest->state != CLIENT_STATE_NICKNAME)
        _server_send_msg_me (s, from, dest, msg);
      l = l->next;
    }
    pthread_mutex_unlock (&(s->m_users));
  }
  /* Send only to 'to' */
  else {
    if (to->fd != -1) {
      pthread_mutex_lock (&(to->m_tx_buff));
      len = snprintf (to->tx_buff, sizeof (to->tx_buff), "%02i:%02i:%02i %s*%s %s%s\n", t->tm_hour, t->tm_min, t->tm_sec, CGRN, from->nickname, msg, CNRM);
      write (to->fd, to->tx_buff, len);
      pthread_mutex_unlock (&(to->m_tx_buff));
    }
  }
}

int
_server_recv_msg (
    Server *s, 
    Client *c)
{
  ssize_t len = 0;
  len = recv (c->fd, c->rx_buff, sizeof (c->rx_buff), 0);

  if (len <= 0) {
    c->fd = -1;
    return CLIENT_STATE_END;
  }

  c->rx_buff[len - 1] = '\0'; // Strip \n

  return CLIENT_STATE_PARSE;
}

int
_server_parse_msg (
    Server *s, 
    Client *c)
{
  if (strncmp (c->rx_buff, "/nick", 5) == 0 && strncmp (c->rx_buff, "/register", 9) == 0) {
    return CLIENT_STATE_NICKNAME;
  }

  if (strncmp (c->rx_buff, "/msg", 4) == 0) {
    return CLIENT_STATE_UNICAST;
  }

  if (strcmp (c->rx_buff, "/usage") == 0 || strcmp (c->rx_buff, "/help") == 0) {
    return CLIENT_STATE_USAGE;
  }

  if (strncmp (c->rx_buff, "/me", 3) == 0) {
    return CLIENT_STATE_ME;
  }

  if (strcmp (c->rx_buff, "/list") == 0) {
    return CLIENT_STATE_LIST;
  }

  if (strcmp (c->rx_buff, "/quit") == 0 || strcmp (c->rx_buff, "/bye") == 0) {
    return CLIENT_STATE_END;
  }

  return CLIENT_STATE_BROADCAST;
}

int   
_server_parse_nickname (
    Server *s, 
    Client *c)
{
  char *oldNickname = c->nickname;

  c->nickname = strdup (c->rx_buff + 6); // '/nick <nickname>'
  c->nickname[strlen(c->nickname)] = '\0';

  char *msg;
  asprintf(&msg, "'%s' changed his nickname to '%s'", oldNickname, c->nickname);
  _server_log (msg);
  _server_send_broadcast_from_server (s, msg);
  free(msg);

  free (oldNickname);

  return CLIENT_STATE_WAITING;
}

void
_server_send_usage (
    Server *s,
    Client *c)
{
  char *msg;

  asprintf (&msg, "Hey %s! Here are the available commands :", c->nickname);
  _server_send_msg_from_server (s, c, msg);

  _server_send_msg_from_server (s, c, "  /nick <nickname>, /register <nickname>   : change your nickname");
  _server_send_msg_from_server (s, c, "  /msg <nickname> <msg>  : sends a private message to <nickname>");
  _server_send_msg_from_server (s, c, "  /list                  : lists currently logged on users");
  _server_send_msg_from_server (s, c, "  /me <msg>              : Execute a virtual action");
  _server_send_msg_from_server (s, c, "  /usage, /help          : print this list of comamnds");
  _server_send_msg_from_server (s, c, "  /bye, /quit            : bye bye!");

  free(msg);
}

void
server_free (
    Server *s)
{
  free (s);
}

void
_server_log (
    char *msg)
{
  time_t timestamp;
  struct tm *t;
  timestamp = time(NULL);
  t = localtime (&timestamp);

  printf ("%02i:%02i:%02i %s\n", t->tm_hour, t->tm_min, t->tm_sec, msg);
}

