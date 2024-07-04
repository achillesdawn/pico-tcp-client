#include "string.h"
#include "pico/cyw43_arch.h"


#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "tcp_client.h"


const char TCP_SERVER_IP[] = "192.168.2.19";
const u16_t TCP_PORT = 9988;

static err_t tcp_client_sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    printf("ON SENT CALLBACK\n\n");
    printf("sent %d", len);
    TCP_CLIENT_T* client = (TCP_CLIENT_T*)arg;
    client->complete = true;
    return ERR_OK;
}

static err_t tcp_client_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    printf("RECEIVED DATA\n");
    return ERR_OK;
}

err_t tcp_client_on_connected(void* arg, struct tcp_pcb* tpcb, err_t err) {
    TCP_CLIENT_T* client = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return err;
    }

    printf("CONNECTED TO SERVER\n\n");

    client->connected = true;
    return ERR_OK;
}

TCP_CLIENT_T* tcp_client_init(void) {
    TCP_CLIENT_T* client = calloc(1, sizeof(TCP_CLIENT_T));
    if (!client) {
        printf("failed to allocate state\n");
        return NULL;
    }

    client->tcp_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!client->tcp_pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(client->tcp_pcb, client);
    tcp_sent(client->tcp_pcb, tcp_client_sent);
    tcp_recv(client->tcp_pcb, tcp_client_recv);

    ip4addr_aton(TCP_SERVER_IP, &client->remote_addr);

    strcpy(client->buffer, "picopicopicopico\n");
    client->buffer_len = strlen(client->buffer);

    return client;
}

bool tcp_client_connect(TCP_CLIENT_T* client) {



    printf("Connecting to %s port %d\n", ip4addr_ntoa(&client->remote_addr), TCP_PORT);
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(client->tcp_pcb, &client->remote_addr, TCP_PORT, tcp_client_on_connected);
    if (err != ERR_OK) {
        printf("CONNECT ERROR, error number %d", err);
        return false;
    }

    tcpwnd_size_t bufsize = client->tcp_pcb->snd_buf;
    printf("send buf size %d", bufsize);

    err = tcp_write(client->tcp_pcb, client->buffer, client->buffer_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("WRITE ERROR CODE %d", err);
        return false;
    }

    bufsize = client->tcp_pcb->snd_buf;
    printf("send buf size %d", bufsize);

    err = tcp_output(client->tcp_pcb);
    if (err != ERR_OK) {
        printf("TCP OUTPUT ERR CODE %d", err);
        return false;
    }
    cyw43_arch_lwip_end();

    bufsize = client->tcp_pcb->snd_buf;
    printf("send buf size %d", bufsize);

    while (true) {
        if (!client->tcp_pcb->connected) {
            break;
        }
        else {
            printf("waiting\n");
            tcpwnd_size_t bufsize = client->tcp_pcb->snd_buf;
            printf("send buf size %d", bufsize);
            sleep_ms(1000);
        }
    }
    printf("WROTE TO BUFFER");


    return true;
}