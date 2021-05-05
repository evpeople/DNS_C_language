#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 600
#define BUFFERS_NUM 19
static unsigned char send_buffers[BUFFERS_NUM + 1][PKT_LEN];
static unsigned char frame_expected = 0;
static unsigned int number_of_send = 0;
static unsigned char have_ack[BUFFERS_NUM + 1];
static unsigned char have_arrived[BUFFERS_NUM + 1];
static bool phl_ready = false;

#define inc(k)           \
    if (k < BUFFERS_NUM) \
        k = k + 1;       \
    else                 \
        k = 0

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
static void chang_number(unsigned char *);
static bool in_middle(unsigned char, unsigned char, unsigned char);
static void send_nak_frame(unsigned char to_send_nak);
static int minus(int);

int main(int argc, char **argv)
{
    int event, arg;
    int len;
    unsigned char not_recvive = 0;
    bool frame_ok = true;
    struct FRAME to_network;

    protocol_init(argc, argv);
    lprintf("Designed by Jiang Yanjun, build: " __DATE__ "  "__TIME__
            "\n");
    dbg_warning("beofore disable event is %d\n", event);

    disable_network_layer();

    dbg_warning("after disable event is %d\n", event);
    for (;;)
    {
        dbg_warning("numberOFSEND is %d\n", number_of_send);
        dbg_warning("event before is %d\n", event);

        event = wait_for_event(&arg);

        dbg_warning("event is %d\n", event);
        switch (event)
        {
        case NETWORK_LAYER_READY:
            dbg_warning("进入了网络层准备好、\n");
            get_packet(send_buffers[number_of_send]);
            have_ack[number_of_send] = 1;
            send_data_frame(number_of_send);
            inc(number_of_send);
            dbg_warning("完成网络层准备好后的 numOFSEND %d\n", number_of_send);
            break;

        case PHYSICAL_LAYER_READY:
            dbg_warning("进入了物理层准备好、\n");
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
            dbg_event("接收到了帧、\n");
            len = recv_frame((unsigned char *)&to_network, sizeof(to_network));

            if (len < 5 || crc32((unsigned char *)&to_network, len) != 0)
            {
                dbg_event("CRC校验出错！%d\n", to_network.kind);
                frame_ok = false;
                break;
            }

            if (to_network.kind == FRAME_DATA)
            {
                dbg_frame("Recv DATA %d %d, ID %d  frame_expect is %d\n", to_network.seq, to_network.ack, *(short *)to_network.data, frame_expected);

                if (to_network.seq != frame_expected)
                {
                    frame_ok = false;
                }
                else
                {
                    frame_ok = true;
                }

                if (frame_ok)
                {
                    dbg_frame("放入了网络层\n");
                    put_packet(to_network.data, len - 7);

                    frame_expected = to_network.seq + 1;
                    chang_number(&frame_expected);

                    send_ack_frame(to_network.seq);
                    have_arrived[minus(to_network.seq)] = 0;
                    have_arrived[to_network.seq] = 1;
                }
                else if (have_arrived[to_network.seq])
                {
                    send_ack_frame(to_network.seq);
                }
            }

            if (to_network.kind == FRAME_ACK)
            {
                dbg_warning("***************************************收到ACK后的 numOFSEND*************%d\n", number_of_send);
                dbg_event("收到了  ACK  帧， ACK  号是 %d not_receive 是 %d  number_OF_SEND  %d \n", to_network.ack, not_recvive, number_of_send);
                if (in_middle(to_network.ack, not_recvive, number_of_send)) //大于就算成功，能大于说明之前也得到了
                {
                    dbg_warning("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~测试in_middle 成功\n");
                    dbg_warning("#######################到have_ack[not_recvive] = 1;后的 numOFSEND*************%d\n", number_of_send);

                    have_ack[not_recvive] = 0;
                    not_recvive = to_network.ack;

                    dbg_warning("#######################到 have_ack[1 + BUFFERS_NUM]++;后的 numOFSEND*************%d\n", number_of_send);

                    have_ack[1 + BUFFERS_NUM]++;

                    dbg_warning("#######################到后stop_timer(not_recvive);的 numOFSEND*************%d\n", number_of_send);

                    stop_timer(not_recvive);
                    number_of_send = minus(number_of_send);

                    dbg_warning("#######################到后的not_recvive++; numOFSEND*************%d\n", number_of_send);
                    not_recvive++;
                    dbg_warning("#######################到后chang_number(&not_recvive);的 numOFSEND*************%d\n", number_of_send);

                    chang_number(&not_recvive);

                    dbg_warning("#######################到ACK后的 numOFSEND*************%d\n", number_of_send);
                }
                dbg_warning("处理ACK后的 numOFSEND %d\n", number_of_send);
            }
            dbg_warning("完成获取数据后的 numOFSEND %d\n", number_of_send);
            break;

        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            for (size_t i = 0; i <= BUFFERS_NUM; i++)
            {
                dbg_frame("被重发的都有%d 判断一下是否有过ACK%d 是否发送过   \n", i, have_ack[i]);
                if (have_ack[i])
                {
                    send_data_frame(i);
                }
            }
            break;
        }
        dbg_warning("完成swich后的 numOFSEND %d\n", number_of_send);

        if (phl_ready && (have_ack[minus(number_of_send)] == 0))
        {
            dbg_warning("启动了网络层\n");
            dbg_frame("启动了网络层，phlready 是 %d  numberOFSend 是 %d have_ack[number_of_send] = %d\n", phl_ready, number_of_send, have_ack[number_of_send % BUFFERS_NUM]);
            enable_network_layer();
        }
        else
        {
            dbg_warning("关闭了网络层，phlready 是 %d  numberOFSend 是 %d have_ack[number_of_send] = %d  have_ack[1 + BUFFERS_NUM]\n");
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

    dbg_frame("Send DATA %d %d, ID %d\n", temp.seq, temp.ack, *(short *)temp.data);
    put_frame((unsigned char *)&temp, 3 + PKT_LEN);

    start_timer(to_send, DATA_TIMER);
}

static void send_ack_frame(unsigned char to_send_ack)
{
    struct FRAME temp;
    temp.kind = FRAME_ACK;
    temp.ack = to_send_ack;

    dbg_frame("Send ACK  %d\n", temp.ack);
    put_frame((unsigned char *)&temp, 2);
}
static void send_nak_frame(unsigned char to_send_nak)
{
    struct FRAME temp;
    temp.kind = FRAME_NAK;
    temp.ack = to_send_nak;

    dbg_frame("Send NAK  %d\n", temp.ack);
    put_frame((unsigned char *)&temp, 2);
}
static void chang_number(unsigned char *t)
{
    if (*t > BUFFERS_NUM)
    {
        *t = 0;
    }
}
static bool in_middle(unsigned char a, unsigned char b, unsigned char c)
{
    if (((a <= b) && (b < c)) || ((b < c) && (c <= a)) || ((c < a) && (a <= b)))
    {
        return true;
    }
    return false;
}
static int minus(int a)
{
    if (a == 0)
    {
        return BUFFERS_NUM;
    }
    else
    {
        return (a - 1);
    }
}