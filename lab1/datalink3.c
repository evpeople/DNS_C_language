#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 2000
#define BUFFERS_NUM 10
static unsigned char send_buffers[BUFFERS_NUM][PKT_LEN];
static unsigned char frame_expected = 0;
static unsigned char number_of_send = 0;
static unsigned char frame_expected = 0;
static unsigned char get_buffer[PKT_LEN];
static unsigned char have_ack[BUFFERS_NUM + 1];

static bool phl_ready = false;

struct FRAME
{
    unsigned char kind; /* FRAME_DATA */
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};
static void put_frame(unsigned char *frame, int len);
static void send_data_frame(unsigned char);
static void send_ack_frame(unsigned char);
int main(int argc, char **argv)
{
    int event, arg;
    int len; //帧的长度？
    int not_recvive = 0;
    struct FRAME to_network;
    struct FRAME to_phl;

    protocol_init(argc, argv); //初始化物理层信道以及debug参数
    lprintf("Designed by Jiang Yanjun, build: " __DATE__ "  "__TIME__
            "\n");

    disable_network_layer();
    bool frame_ok = true;
    for (;;)
    {
        event = wait_for_event(&arg); //arg 是超时的计时器的编号
        switch (event)
        {
        case NETWORK_LAYER_READY: //可以发送新的数据了（可以从网络层拿包了
            get_packet(send_buffers[number_of_send]);
            have_ack[number_of_send] = 0;
            send_data_frame(number_of_send);
            number_of_send++;
            break;
        case PHYSICAL_LAYER_READY:
            phl_ready = 1;
            break;
        case FRAME_RECEIVED: //收到了帧，需要发送到网络层
            len = recv_frame((unsigned char *)&to_network, sizeof(to_network));
            if (len < 5 || crc32((unsigned char *)&to_network, len) != 0)
            {
                dbg_event("CRC校验出错！");
                frame_ok = false;
            }
            if (to_network.kind == FRAME_DATA)
            {
                if (to_network.seq != frame_expected)
                {
                    frame_ok = false;
                }

                if (frame_ok)
                {
                    put_packet(to_network.data, len - 7);
                    //此处先不考虑捎带确认。
                    if (frame_expected > BUFFERS_NUM)
                    {
                        frame_expected = 0;
                    }
                    else
                    {
                        frame_expected++;
                    }
                    send_ack_frame(to_network.seq);
                }
            }
            if (to_network.kind == FRAME_ACK)
            {
                if (to_network.ack == not_recvive)
                {
                    have_ack[not_recvive] = 1;
                    have_ack[1 + BUFFERS_NUM]++;
                    stop_timer(not_recvive);
                    not_recvive++;
                    if (not_recvive > BUFFERS_NUM)
                    {
                        not_recvive = 0;
                    }
                }
            }
            break;
        case DATA_TIMEOUT:
            for (size_t i = arg; i < BUFFERS_NUM; i++)
            {
                send_data_frame(i);
            }
            break;
        case ACK_TIMEOUT:
            break;
        }
        if (number_of_send > BUFFERS_NUM)
        {
            number_of_send = 0;
        }

        //物理层准备好，缓冲没有满，待发送的位置的ack已经到了
        if (phl_ready && number_of_send <= BUFFERS_NUM && (have_ack[number_of_send] == 1 || have_ack[1 + BUFFERS_NUM] > BUFFERS_NUM))
        {
            enable_network_layer();
        }
        else
        {
            disable_network_layer();
        }
    }
}
static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

static void send_data_frame(unsigned char to_send)
{
    struct FRAME temp;

    temp.seq = to_send;
    temp.kind = FRAME_DATA;

    memcpy(temp.data, send_buffers[to_send], PKT_LEN);

    put_frame((unsigned char *)&temp, 3 + PKT_LEN);

    start_timer(to_send, DATA_TIMER);
}

static void send_ack_frame(unsigned char to_send_ack)
{
    struct FRAME temp;
    temp.kind = FRAME_ACK;
    temp.ack = to_send_ack;

    put_frame((unsigned char *)&temp, 2);
}