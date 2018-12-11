#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define QSIZE 10   /* キューの長さ*/
#define LEN 3      /* データ配列の長さ */
#define PUT_NUM 30 /* 追加するエントリ数 */
#define GET_NUM 10 /* 取り出すエントリ数 */

typedef struct
{
    pthread_mutex_t buf_lock; /* 構造体のロック*/
    int start;                /* バッファの開始*/
    int num_full;             /* データの数*/
    pthread_cond_t notfull;   /* notfullの条件変数*/
    pthread_cond_t notempty;  /* notemptyの条件変数*/
    void *data[QSIZE];        /* 巡回バッファ*/
} circ_buf_t;

/* 巡回バッファのポインタ */
circ_buf_t cb;
circ_buf_t *cbp = &cb;

void put_cb_data(circ_buf_t *cbp, void *data)
{
    pthread_mutex_lock(&cbp->buf_lock);

    while (cbp->num_full == QSIZE)
        pthread_cond_wait(&cbp->notfull, &cbp->buf_lock);
    cbp->data[(cbp->start + cbp->num_full) % QSIZE] = data;
    cbp->num_full++;

    pthread_cond_signal(&cbp->notempty);
    pthread_mutex_unlock(&cbp->buf_lock);
}

void *get_cb_data(circ_buf_t *cbp)
{
    void *data;
    pthread_mutex_lock(&cbp->buf_lock);
    while (cbp->num_full == 0)
        pthread_cond_wait(&cbp->notempty, &cbp->buf_lock);
    data = cbp->data[cbp->start];
    cbp->start = (cbp->start + 1) % QSIZE;
    cbp->num_full--;
    pthread_cond_signal(&cbp->notfull);
    pthread_mutex_unlock(&cbp->buf_lock);
    return (data);
}

/* 初期化の関数 */
void init(circ_buf_t *cbp)
{
    pthread_mutex_init(&cbp->buf_lock, NULL);
    cbp->start = 0;
    cbp->num_full = 0;
    pthread_cond_init(&cbp->notempty, NULL);
    pthread_cond_init(&cbp->notfull, NULL);
}

/* 関数ポインタと引数のポインタを入力 */
void *put(void *arg)
{
    /* 入力するデータを設定 */
    int data[LEN];
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    /* 追加する */
    for (int i = 0; i < PUT_NUM; i++)
    {
        put_cb_data(cbp, data);
    }
    /* 表示 */
    printf("Put data :");
    for (int i = 0; i < LEN; i++)
    {
        printf(" %d", data[i]);
    }
    printf("\n");
    sleep(1);
}

/* 指定した回数だけ取り出す */
void get(void *arg)
{
    int *result = (int *)malloc(LEN * sizeof(int));
    /* 取り出す */
    for (int i = 0; i < GET_NUM; i++)
    {
        result = get_cb_data(cbp);
    }
    /* 表示 */
    printf("Get data :");
    for (int i = 0; i < LEN; i++)
    {
        printf(" %d", result[i]);
    }
    printf("\n");
    sleep(1);
}

void *test(void *arg) {
    int value = (int *)arg;
    printf("%d", value);
}

int main(int argc, char const *argv[])
{
    /* 初期化 */
    init(cbp);

    pthread_t put1, put2;
    pthread_t get1, get2, get3, get4, get5, get6;

    /* 追加するスレッドを2つ作成 */
    pthread_create(&put1, NULL, (void *)put, NULL);
    pthread_create(&put2, NULL, (void *)put, NULL);

    /* 取り出すスレッドを6つ作成 */
    pthread_create(&get1, NULL, (void *)get, NULL);
    pthread_create(&get2, NULL, (void *)get, NULL);
    pthread_create(&get3, NULL, (void *)get, NULL);
    pthread_create(&get4, NULL, (void *)get, NULL);
    pthread_create(&get5, NULL, (void *)get, NULL);
    pthread_create(&get6, NULL, (void *)get, NULL);

    pthread_join(put1, NULL);
    pthread_join(put2, NULL);
    pthread_join(get1, NULL);
    pthread_join(get2, NULL);
    pthread_join(get3, NULL);
    pthread_join(get4, NULL);
    pthread_join(get5, NULL);
    pthread_join(get6, NULL);

    return 0;
}
