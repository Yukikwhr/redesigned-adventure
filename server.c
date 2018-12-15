#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "logutil.h"

#include <pthread.h>
#include <signal.h>

#define THREAD_POOL_NUM 3

#define DEFAULT_SERVER_PORT 10000
#ifdef SOMAXCONN
#define LISTEN_BACKLOG SOMAXCONN
#else
#define LISTEN_BACKLOG 5
#endif

// キュー
struct entry
{
    struct entry *next;
    void *(*func)(void *arg);
    void *data;
};

struct list
{
    pthread_mutex_t data_lock;
    pthread_cond_t notempty;
    struct entry *head;
    struct entry **tail;
};

struct list *list_init(void)
{
    struct list *list;

    list = malloc(sizeof *list);
    if (list == NULL)
        return (NULL);
    list->head = NULL;
    list->tail = &list->head;
    pthread_mutex_init(&list->data_lock, NULL);
    pthread_cond_init(&list->notempty, NULL);
    return (list);
}

int list_enqueue(struct list *list, void *data, void *(*func)(void *arg))
{
    struct entry *e;

    e = malloc(sizeof *e);
    if (e == NULL)
        return (1);
    e->next = NULL;
    e->data = data;
    e->func = func;
    pthread_mutex_lock(&list->data_lock);
    *list->tail = e;
    list->tail = &e->next;
    pthread_cond_signal(&list->notempty);
    pthread_mutex_unlock(&list->data_lock);
    return (0);
}

struct entry *list_dequeue(struct list *list)
{
    struct entry *e;

    pthread_mutex_lock(&list->data_lock);
    while (list->tail == &list->head)
    {
        pthread_cond_wait(&list->notempty, &list->data_lock);
    }
    if (list->head == NULL)
    {
        pthread_mutex_unlock(&list->data_lock);
        return (NULL);
    }
    e = list->head;
    list->head = e->next;
    if (list->head == NULL)
    {
        list->tail = &list->head;
    }
    pthread_mutex_unlock(&list->data_lock);
    return (e);
}

// サーバー
char *program_name = "server";

int open_accepting_socket(int port)
{
    struct sockaddr_in self_addr;
    socklen_t self_addr_size;
    int sock, sockopt;

    memset(&self_addr, 0, sizeof(self_addr));
    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = INADDR_ANY;
    self_addr.sin_port = htons(port);
    self_addr_size = sizeof(self_addr);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        logutil_fatal("accepting socket: %d", errno);
    sockopt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                   &sockopt, sizeof(sockopt)) == -1)
        logutil_warning("SO_REUSEADDR: %d", errno);
    if (bind(sock, (struct sockaddr *)&self_addr, self_addr_size) < 0)
        logutil_fatal("bind accepting socket: %d", errno);
    if (listen(sock, LISTEN_BACKLOG) < 0)
        logutil_fatal("listen: %d", errno);
    return (sock);
}

/* キューに貯める関数ポインタ */
void *protocol_main(void *arg)
{
    int *sock = (int *)arg;
    logutil_info("connected");
    char c[1];
    while (read(*sock, c, sizeof c) > 0)
        /* ignore protocol process */;
    logutil_info("disconnected");
}

/* クリーンアップハンドラー */
void cleanup(void *arg)
{
    struct entry *entry = (struct entry *)arg;
    logutil_info("cancel");
    free(entry);
}

/* ワーカー*/
void *worker(void *arg)
{
    struct list *list = (struct list *)arg;
    struct entry *entry;

    for (;;)
    {
        entry = list_dequeue(list);
        pthread_cleanup_push(cleanup, entry);
        (*entry->func)(entry->data);
        pthread_cleanup_pop(1);
    }
}

/* リクエスト取得，エンキュー */
void main_loop(int accepting_socket, struct list *list)
{
    int sock;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    for (;;)
    {
        client_addr_size = sizeof(client_addr);
        sock = accept(accepting_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (sock < 0)
        {
            logutil_error("accept error : % d", errno);
        }
        else
        {
            list_enqueue(list, (void *)&sock, protocol_main);
        }
    }
}

void usage(void)
{
    fprintf(stderr, "Usage: %s [option]\n", program_name);
    fprintf(stderr, "option:\n");
    fprintf(stderr, "\t-d\t\t\t\t... debug mode\n");
    fprintf(stderr, "\t-p <port>\n");
    exit(1);
}

sigset_t sig;

/* シグナル処理をする関数 */
void *handle_sig(void *arg)
{
    int err, local_sig;
    err = sigwait(&sig, &local_sig);
    if (err || (local_sig != SIGINT && local_sig != SIGTERM))
    {
        abort();
    }
    logutil_notice("bye");
    return (NULL);
}

int main(int argc, char **argv)
{
    char *port_number = NULL;
    int ch, sock, server_port = DEFAULT_SERVER_PORT;
    int debug_mode = 0;

    while ((ch = getopt(argc, argv, "dp:")) != -1)
    {
        switch (ch)
        {
        case 'd':
            debug_mode = 1;
            break;
        case 'p':
            port_number = optarg;
            break;
        case '?':
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (port_number != NULL)
        server_port = strtol(port_number, NULL, 0);

    /* server_portでlistenし，socket descriptorをsockに代入 */
    sock = open_accepting_socket(server_port);

    if (!debug_mode)
    {
        logutil_syslog_open(program_name, LOG_PID, LOG_LOCAL0);
        daemon(0, 0);
    }

    /* SIGINTとSIGTERMをブロック */
    sigemptyset(&sig);
    sigaddset(&sig, SIGINT);
    sigaddset(&sig, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sig, NULL);

    /* シグナル制御用スレッド生成 */
    pthread_t sig_p;
    pthread_create(&sig_p, NULL, handle_sig, NULL);

    struct list *list = list_init();

    /* ワーカスレッド生成 */
    pthread_t work[THREAD_POOL_NUM];
    for (int i = 0; i < THREAD_POOL_NUM; i++)
    {
        pthread_create(&work[i], NULL, worker, list);
    }

    main_loop(sock, list);

    for (int i = 0; i < THREAD_POOL_NUM; i++)
    {
        pthread_cancel(work[i]);
    }

    for (int i = 0; i < THREAD_POOL_NUM; i++)
    {
        pthread_join(work[i], NULL);
    }

    /*NOTREACHED*/
    return (0);
}