#include "stun_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_check.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_random.h"

static const char *TAG = "stun_client";

static int sockfd;

static void frame_stun(uint8_t *buffer){
    stun_msg_t * msg = (stun_msg_t *)buffer;
    msg->type       = htons(0x0001);
    msg->length     = htons(0x0000);
    msg->magic      = htonl(MAGIC_COOKIE);
    msg->payload[0] = esp_random();
    msg->payload[1] = esp_random();
    msg->payload[2] = esp_random();
}

static esp_err_t parse_response(uint8_t *buf, uint8_t size){
    int type;
    int length;
    short port;
    int i = 20;
    if (*(short *)(&buf[0]) == htons(0x0101))
    {
        while(i < size){
            type   = htons(*(short *)(&buf[i]));
            length = htons(*(short *)(&buf[i+2]));
            switch(type) {
                case MAPPED_ADDRESS:
                    port = ntohs(*(short *)(&buf[i+6]));
                    ESP_LOGI(TAG,"IP %d.%d.%d.%d:%d",buf[i+8],buf[i+9],buf[i+10],buf[i+11],port);
                    break;
                case XOR_MAPPED_ADDRESS:
                    port = ntohs(*(short *)(&buf[i+6]));
                    port ^= 0x2112;
                    ESP_LOGI(TAG,"IP %d.%d.%d.%d:%d",buf[i+8]^0x21,buf[i+9]^0x12,buf[i+10]^0xA4,buf[i+11]^0x42,port);
                    break;
                case SOFTWARE:
                    ESP_LOGI(TAG,"SOFTWARE:%s", &buf[i+4]);
                    break;
                default:
                    ESP_LOGD(TAG,"ignore: %#X", type);
            }
            i += (4  + length);
        }
    }
    return ESP_OK;
}


esp_err_t stun_client_opt(stun_context *stun){
    esp_err_t ret = ESP_OK;
    int resive;
    uint8_t buf[200];
    uint8_t request[20];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family =  AF_INET;
    inet_pton(AF_INET, stun->stun_server , &servaddr.sin_addr);
    servaddr.sin_port   = htons(stun->stun_port);

    struct sockaddr_in localaddr;
    bzero(&localaddr, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0" , &localaddr.sin_addr);
    localaddr.sin_port = htons(stun->local_port);

    struct timeval tv = {
        .tv_sec = 10
    };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr));

    frame_stun(request);

    ESP_GOTO_ON_FALSE(sendto(sockfd, request, 20, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)), ESP_FAIL, end, TAG, "sendto failed");
    ESP_GOTO_ON_FALSE(resive = recvfrom(sockfd, buf, 200, 0, NULL, 0), ESP_ERR_TIMEOUT, end, TAG, "recvfrom failed");

    parse_response(buf, resive);

end:
    close(sockfd);
    return ret;
}

esp_err_t stun_client(){
    struct hostent  *res;
    char host[50];
    int port = -1;
    int rnd = esp_random()%list_server_count;

    sscanf(list_server[rnd], "%[^:]:%d", host, &port);
    res = gethostbyname(host);
    if (res) {
        ESP_LOGI(TAG, "host %s, resolved to: %s", host, inet_ntoa(*(struct in_addr *) (res->h_addr_list[0])));
        stun_context target = {
            .stun_server    = inet_ntoa(*(struct in_addr *) (res->h_addr_list[0])),
            .stun_port      = port,
            .local_port     = 1234,
        };
        ESP_LOGI(TAG,"server %s port %d", target.stun_server, target.stun_port);
        return stun_client_opt(&target);
    }

    return ESP_FAIL;
}

