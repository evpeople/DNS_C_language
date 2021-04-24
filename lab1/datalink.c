#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//遇见的问题，fileB4是所有的问题，大概就是丢了ACK之后，会导致原本的发送方一直发送实际上已经到达的消息，导致死锁，
#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 1000
#define MAX_SEQ 7
#define BUFFERS_NUM 10
static unsigned char send_buffers[BUFFERS_NUM][PKT_LEN];
static unsigned int number_of_send = 0;
static unsigned char get_buffer[PKT_LEN];
static unsigned char have_ack[BUFFERS_NUM + 1];
static unsigned char have_send[BUFFERS_NUM + 1];
static unsigned char have_arrived[BUFFERS_NUM + 1];

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
static void send_data_frame(unsigned char frame_nr, unsigned char frame_expected, char number_of_buffer);
static void send_ack_frame(unsigned char);
static void chang_number(int *);
static bool in_middle(unsigned char, unsigned char, unsigned char);
static void send_nak_frame(unsigned char to_send_nak);

int main(int argc, char **argv)
{
    int next_frame_to_send, ack_expected, frame_expected;
    struct FRAME r;
    int nbuufered, i;
    int event;
    ack_expected = 0;
    next_frame_to_send = 0;
    frame_expected = 0;
    nbuufered = 0;
    int arg;
    int len;
    protocol_init(argc, argv); //初始化物理层信道以及debug参数
    lprintf("Designed by Jiang Yanjun, build: " __DATE__ "  "__TIME__
            "\n");
    dbg_warning("beofore disable event is %d\n", event);

    enable_network_layer();
    dbg_warning("after disable event is %d\n", event);
    bool frame_ok = true;
    for (;;)
    {
        event = wait_for_event(&arg); //arg 是超时的计时器的编号
        switch (event)
        {
        case NETWORK_LAYER_READY: //可以发送新的数据了（可以从网络层拿包了
            dbg_warning("进入了网络层准备好、\n");
            get_packet(send_buffers[next_frame_to_send]);
            nbuufered++;

            send_data_frame(next_frame_to_send, frame_expected, next_frame_to_send);
            chang_number(&next_frame_to_send);
            // have_send[number_of_send % BUFFERS_NUM] = 1;
            dbg_event(" next_frame_to send =%d\n", next_frame_to_send);
            // have_ack[number_of_send % BUFFERS_NUM] = 0;
            // send_data_frame(number_of_send % BUFFERS_NUM);
            // number_of_send++;

            // dbg_error("完成网络层准备好后的 numOFSEND %d\n", number_of_send);
            break;
        case PHYSICAL_LAYER_READY:
            dbg_warning("进入了物理层准备好、\n");
            phl_ready = 1;
            break;
        case FRAME_RECEIVED: //收到了帧，需要发送到网络层
            dbg_event("接收到了帧、\n");
            len = recv_frame((unsigned char *)&r, sizeof(r));

            if (len < 5 || crc32((unsigned char *)&r, len) != 0)
            {
                dbg_event("CRC校验出错！%d\n");
                break;
            }
            dbg_frame("Recv DATA %d %d, ID %d\n frame_expect=%d \n", r.seq, r.ack, *(short *)r.data, frame_expected);
            if (r.kind == FRAME_DATA)
            {
                if (r.seq == frame_expected)
                {
                    dbg_event("放入网络层\n");
                    put_packet(r.data, len - 7);
                    chang_number(&frame_expected);
                }

                while (in_middle(ack_expected, r.ack, next_frame_to_send))
                {
                    nbuufered--;
                    stop_timer(ack_expected);
                    chang_number(&ack_expected);
                }
            }
            break;
        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            next_frame_to_send = ack_expected;
            for (size_t i = 0; i < nbuufered; i++)
            {
                // dbg_frame("have_ack = %d\n", have_ack[i]);
                send_data_frame(next_frame_to_send, frame_expected, next_frame_to_send);
                chang_number(&next_frame_to_send);
            }
            break;
            dbg_error("完成超时后的 numOFSEND %d\n", number_of_send);
        case ACK_TIMEOUT:
            break;
        }
        dbg_error("完成swich后的 numOFSEND %d\n", number_of_send);
        dbg_warning("完成了switch\n");
        //物理层准备好，缓冲没有满，待发送的位置的ack已经到了
        if (phl_ready && nbuufered < MAX_SEQ)
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
        // chang_number(&number_of_send);
    }
}

static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

static void send_data_frame(unsigned char frame_nr, unsigned char frame_expected, char number_of_buffer)
{
    struct FRAME temp;

    temp.seq = frame_nr;
    temp.kind = FRAME_DATA;
    temp.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    memcpy(temp.data, send_buffers[number_of_buffer], PKT_LEN);

    dbg_frame("Send DATA %d %d, ID %d\n", temp.seq, temp.ack, *(short *)temp.data);
    put_frame((unsigned char *)&temp, 3 + PKT_LEN);

    start_timer(frame_nr, DATA_TIMER);
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
static void chang_number(int *t)
{
    if (*t < MAX_SEQ)
    {
        (*t)++;
    }
    else
    {
        *t = 0;
    }
}
static bool in_middle(unsigned char a, unsigned char b, unsigned char c)
{
    if (((a <= b) && (b < c)) || ((b <= c) && (c < a)) || ((c < a) && (a <= b)))
    {
        return true;
    }
    return false;
}