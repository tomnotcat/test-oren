#include <glib.h>

#ifdef G_OS_WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#include <oren-ncbuffer.h>
#include <oren-nchandler.h>
#include <oren-ncreactor.h>
#include <oren-ncsocket.h>
#include <oren-ncutils.h>

#define BUFLEN 5120
#define PKTLEN 1200
#define PORT 9494

static gchar *test_mode = "common";
static gchar *server_ip = "122.227.23.137";
static guint server_port = 9494;
static guint seq = 0;

static void _common_run (void)
{
    struct sockaddr_in si_me, si_other;
    int s, l, slen = sizeof (si_other);
    char buf[BUFLEN] = { 0 };

#ifdef G_OS_WIN32
    WSADATA wsaData;
    if (WSAStartup (MAKEWORD(2, 2), &wsaData) != 0)
        g_error ("WSAStartup");
#endif

    if ((s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        g_error ("socket");

    memset ((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = 0;
    si_me.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (s, (const struct sockaddr *)&si_me, sizeof (si_me)) == -1)
        g_error ("bind");

    si_other.sin_family = AF_INET;
    si_other.sin_port = htons (server_port);
#ifdef G_OS_WIN32
    si_other.sin_addr.s_addr = inet_addr (server_ip);
#else
    if (inet_aton (server_ip, &si_other.sin_addr) == 0)
        g_error ("inet_aton");
#endif

    while (1) {
        if (sendto (s, buf, PKTLEN, 0,
                    (const struct sockaddr*)&si_other, slen) != PKTLEN)
        {
            g_error ("sendto");
        }

        l = recvfrom (s, buf, BUFLEN, 0, (struct sockaddr*)&si_other, &slen);
        if (l != PKTLEN)
            g_error ("recvfrom");

        g_assert (PKTLEN == l);

        g_print ("Common packet from %s:%d:%u\n",
                 inet_ntoa (si_other.sin_addr),
                 ntohs (si_other.sin_port),
                 seq++);

        g_usleep (10000);
    }

#ifdef G_OS_WIN32
    closesocket (s);
    WSACleanup ();
#else
    close (s);
#endif
}

static void _handle_packet (OrenNCHandler *self,
                            OrenNCSocket *socket,
                            OrenNCSockaddr *from,
                            OrenNCBuffer *buffer)
{
    gchar *str = oren_ncsockaddr_to_string (from);

    g_assert (oren_ncbuffer_unread_length (buffer) == PKTLEN);

    g_print ("Oren packet from %s:%u\n", str, seq++);

    g_free (str);
}

static void _handle_timer (OrenNCHandler *self,
                           guint timer_id,
                           OrenNCSocket *socket)
{
    gchar buf[PKTLEN];
    OrenNCSockaddr *addr;

    addr = oren_ncsockaddr_new_simple (server_ip, server_port);
    oren_ncsocket_send_to (socket, addr, buf, PKTLEN);
    g_object_unref (addr);
}

static void _oren_run (void)
{
    OrenNCReactor *reactor;
    OrenNCSocket *socket;
    OrenNCHandler *handler;

    reactor = oren_ncreactor_new ();
    socket = oren_ncsocket_new_datagram (NULL, 0);
    handler = oren_nchandler_new ();

    g_signal_connect (handler,
                      "handle-packet",
                      G_CALLBACK (_handle_packet),
                      NULL);

    g_signal_connect (handler,
                      "handle-timer",
                      G_CALLBACK (_handle_timer),
                      socket);

    oren_ncreactor_register_handler (reactor,
                                     socket,
                                     handler);
    oren_ncreactor_schedule_timer (reactor,
                                   10,
                                   handler);
    oren_ncreactor_run_loop (reactor, FALSE);
}

int main (int argc, char *argv[])
{
    static GOptionEntry entries[] = {
        { "test-mode", 'm', 0, G_OPTION_ARG_STRING, &test_mode, "Test mode", NULL },
        { "server-ip", 'i', 0, G_OPTION_ARG_STRING, &server_ip, "Server IPs", NULL },
        { "server-port", 'p', 0, G_OPTION_ARG_INT, &server_port, "Server port", NULL },
        { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;

    g_type_init ();

    context = g_option_context_new ("test-oren");
    g_option_context_add_main_entries (context, entries, "");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_warning ("Option parsing failed: %s", error->message);
        g_clear_error (&error);
    }

    g_option_context_free (context);

    if (strcmp (test_mode, "common") == 0) {
        _common_run ();
    }
    else {
        _oren_run ();
    }

    return 0;
}
