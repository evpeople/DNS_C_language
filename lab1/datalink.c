#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//遇见的问题，fileB4是所有的问题，大概就是丢了ACK之后，会导致原本的发送方一直发送实际上已经到达的消息，导致死锁，
#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 2000
#define BUFFERS_NUM 10
static unsigned char send_buffers[BUFFERS_NUM + 1][PKT_LEN];
static unsigned char frame_expected = 0;
static unsigned int number_of_send = 0;
static unsigned char get_buffer[PKT_LEN];
static unsigned char have_ack[BUFFERS_NUM + 1];
static unsigned char have_send[BUFFERS_NUM + 1];
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

int main(int argc, char **argv)
{
    int event, arg;
    int len; //帧的长度？
    unsigned char not_recvive = 0;
    struct FRAME to_network;
    struct FRAME to_phl;

    protocol_init(argc, argv); //初始化物理层信道以及debug参数
    lprintf("Designed by Jiang Yanjun, build: " __DATE__ "  "__TIME__
            "\n");
    dbg_warning("beofore disable event is %d\n", event);

    disable_network_layer();
    dbg_warning("after disable event is %d\n", event);
    bool frame_ok = true;
    for (;;)
    {
        dbg_error("numberOFSEND is %d\n", number_of_send);
        dbg_warning("event before is %d\n", event);
        event = wait_for_event(&arg); //arg 是超时的计时器的编号
        dbg_warning("event is %d\n", event);
        switch (event)
        {
        case NETWORK_LAYER_READY: //可以发送新的数据了（可以从网络层拿包了
            dbg_warning("进入了网络层准备好、\n");
            get_packet(send_buffers[number_of_send]);
            have_send[number_of_send] = 1;
            have_ack[number_of_send] = 0;
            send_data_frame(number_of_send);
            // number_of_send++;
            inc(number_of_send);
            dbg_error("完成网络层准备好后的 numOFSEND %d\n", number_of_send);
            break;
        case PHYSICAL_LAYER_READY:
            dbg_warning("进入了物理层准备好、\n");
            phl_ready = 1;
            break;
        case FRAME_RECEIVED: //收到了帧，需要发送到网络层
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
                    //此处先不考虑捎带确认。
                    //frame_expected++;

                    frame_expected = to_network.seq + 1;
                    chang_number(&frame_expected);
                    send_ack_frame(to_network.seq);
                    have_arrived[to_network.seq] = 1;
                }
                // else if (have_arrived[to_network.seq])
                // {
                //     /* code */
                //     send_ack_frame(to_network.seq);
                // }
            }
            if (to_network.kind == FRAME_ACK)
            {
                dbg_error("***************************************收到ACK后的 numOFSEND*************%d\n", number_of_send);
                dbg_event("收到了  ACK  帧， ACK  号是 %d not_receive 是 %d  number_OF_SEND  %d \n", to_network.ack, not_recvive, number_of_send);
                if (in_middle(to_network.ack, not_recvive, number_of_send)) //大于就算成功，能大于说明之前也得到了
                {
                    dbg_frame("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~测试in_middle 成功\n");
                    dbg_error("#######################到have_ack[not_recvive] = 1;后的 numOFSEND*************%d\n", number_of_send);
                    have_ack[not_recvive] = 1;
                    // for (size_t i = (not_recvive + 1) % BUFFERS_NUM; i <= to_network.ack; i++)
                    // {
                    //     have_ack[i] = 1;
                    // }
                    not_recvive = to_network.ack;
                    dbg_error("#######################到 have_ack[1 + BUFFERS_NUM]++;后的 numOFSEND*************%d\n", number_of_send);
                    have_ack[1 + BUFFERS_NUM]++;
                    dbg_error("#######################到后stop_timer(not_recvive);的 numOFSEND*************%d\n", number_of_send);
                    stop_timer(not_recvive);
                    // number_of_send--;
                    dbg_error("#######################到后的not_recvive++; numOFSEND*************%d\n", number_of_send);
                    not_recvive++;
                    dbg_error("#######################到后chang_number(&not_recvive);的 numOFSEND*************%d\n", number_of_send);
                    chang_number(&not_recvive);
                    dbg_error("#######################到ACK后的 numOFSEND*************%d\n", number_of_send);
                }
                dbg_error("处理ACK后的 numOFSEND %d\n", number_of_send);
            }
            if (to_network.kind == FRAME_NAK)
            {
                //send_data_frame(to_network.ack + 1);
            }

            dbg_error("完成获取数据后的 numOFSEND %d\n", number_of_send);
            break;
        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            for (size_t i = 0; i < BUFFERS_NUM; i++)
            {
                if (number_of_send + 1 >= BUFFERS_NUM)
                {
                    dbg_frame("被重发的都有%d 判断一下是否有过ACK%d \n", i, have_ack[i]);
                    if (!have_ack[i])
                    {
                        // dbg_frame("have_ack = %d\n", have_ack[i]);
                        send_data_frame(i);
                    }
                }
                else
                {
                    send_data_frame(arg);
                    break;
                }
            }
            break;
            dbg_error("完成超时后的 numOFSEND %d\n", number_of_send);
        case ACK_TIMEOUT:
            break;
        }
        dbg_error("完成swich后的 numOFSEND %d\n", number_of_send);
        dbg_warning("完成了switch\n");
        //物理层准备好，缓冲没有满，待发送的位置的ack已经到了
        if (phl_ready && (number_of_send < BUFFERS_NUM || have_ack[(number_of_send - 1) % BUFFERS_NUM] == 1))
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