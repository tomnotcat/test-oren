#include <glib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <oren-ncbuffer.h>
#include <oren-nchandler.h>
#include <oren-ncreactor.h>
#include <oren-ncsocket.h>
#include <oren-ncutils.h>

#define BUFLEN 5120
#define PKTLEN 1200
#define PORT 9494

static gpointer _common_thread (gpointer data)
{
    struct sockaddr_in si_me, si_other;
    int s, l, slen = sizeof (si_other);
    char buf[BUFLEN];

    if ((s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        g_error ("socket");

    memset ((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons (PORT);
    si_me.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (s, (const struct sockaddr *)&si_me, sizeof (si_me)) == -1)
        g_error ("bind");

    while (1) {
        l = recvfrom (s, buf, BUFLEN, 0, (struct sockaddr*)&si_other, &slen);
        if (l == -1)
            g_error ("recvfrom");

        g_assert (PKTLEN == l);

        g_print ("Common packet from %s:%d\n",
                 inet_ntoa (si_other.sin_addr),
                 ntohs (si_other.sin_port));

        if (sendto (s, buf, l, 0, (const struct sockaddr*)&si_other, slen) == -1)
            g_error ("sendto");
    }

    close (s);
    return NULL;
}

static void _handle_packet (OrenNCHandler *self,
                            OrenNCSocket *socket,
                            OrenNCSockaddr *from,
                            OrenNCBuffer *buffer)
{
    gchar *str = oren_ncsockaddr_to_string (from);

    g_assert (oren_ncbuffer_unread_length (buffer) == PKTLEN);

    g_print ("Oren packet from %s\n", str);

    oren_ncsocket_send_to (socket,
                           from,
                           oren_ncbuffer_read_ptr (buffer),
                           oren_ncbuffer_unread_length (buffer));
    g_free (str);
}

static gpointer _oren_thread (gpointer data)
{
    OrenNCReactor *reactor;
    OrenNCSocket *socket;
    OrenNCHandler *handler;

    reactor = oren_ncreactor_new ();
    socket = oren_ncsocket_new_datagram (NULL, 9595);
    handler = oren_nchandler_new ();

    g_signal_connect (handler,
                      "handle-packet",
                      G_CALLBACK (_handle_packet),
                      NULL);

    oren_ncreactor_register_handler (reactor,
                                     socket,
                                     handler);
    oren_ncreactor_run_loop (reactor, FALSE);

    return NULL;
}

int main (int argc, char *argv[])
{
    GThread *t1;
    GThread *t2;

    g_type_init ();

    t1 = g_thread_new ("common",
                       _common_thread,
                       NULL);

    t2 = g_thread_new ("oren",
                       _oren_thread,
                       NULL);

    g_thread_join (t1);
    g_thread_join (t2);
    return 0;
}
