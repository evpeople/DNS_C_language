#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 2000
#define ACK_TIMER 1000

struct FRAME
{
    unsigned char kind; /* FRAME_DATA */
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};

static unsigned char frame_nr = 0, buffer[PKT_LEN], nbuffered; //全局变量，默认为0，frame_nr 在 0 1 之间改变，frame_nr 是上一个发送的帧，但是更大的窗口就不能这样操作了
static unsigned char frame_expected = 0;
static int phl_ready = 0;

static bool start_ack_time = false;
static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

static void send_data_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_DATA;
    s.seq = frame_nr;
    if (start_ack_time)
    {
        s.ack = 1 - frame_expected;
        stop_ack_timer();
        start_ack_time = false;
    }

    memcpy(s.data, buffer, PKT_LEN);

    dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(frame_nr, DATA_TIMER);
}

static void send_ack_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_ACK;
    s.ack = 1 - frame_expected;

    dbg_frame("Send ACK  %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);
}
static void send_nak_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_NAK;
    s.ack = 1 - frame_expected;

    dbg_frame("Send NAK  %d\n", s.ack);
}
int main(int argc, char **argv)
{
    int event, arg;
    struct FRAME f;
    int len = 0;

    protocol_init(argc, argv); //初始化物理层信道以及debug参数
    lprintf("Designed by Jiang Yanjun, build: " __DATE__ "  "__TIME__
            "\n");

    disable_network_layer();

    bool right_frame = true;
    for (;;)
    {
        //未实现ACK定时器，也就是说没有实现捎带确认
        //需要自己实现把帧放到指定的窗口
        event = wait_for_event(&arg);
        switch (event)
        {
        case NETWORK_LAYER_READY:
            get_packet(buffer); //将待发送分组缓冲到buffer中
            nbuffered++;        //缓冲的总数？
            send_data_frame();
            break;

        case PHYSICAL_LAYER_READY:
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
            len = recv_frame((unsigned char *)&f, sizeof f);

            if (len < 5 || crc32((unsigned char *)&f, len) != 0)
            {
                dbg_event("**** Receiver Error, Bad CRC Checksum    kind is %d\n", f.kind);
                //出了问题，我应该通知发送方让他重传。
                right_frame = false;
            }
            if (right_frame == false)
            {
                //收到了错误的帧，我发送了NAK，只添加NAK只能保证更快的回复，不用等待超时,不能显著提高结果
                dbg_event("---- NAK  %d happen\n", arg);
                send_nak_frame();
                right_frame = true;
                break;
            }

            if (f.kind == FRAME_ACK)
                dbg_frame("Recv ACK  %d\n", f.ack);
            dbg_frame("ACK计时器的状态  %d\n", start_ack_time);
            if (f.kind == FRAME_NAK)
            { //此处收到
                dbg_frame("Recv NAK  %d\n", f.ack);

                //不出现是因为没有接受过NAK，当收到NAK之后，我应该直接重传消息，并且停止计时器，重传的帧等价于超时之后的那一帧，
                //遇到的问题是，怎么停止计时器，NAK有可能被丢掉，所以普通的超时还要保存，经过分析之后，其实并没有影响
                send_data_frame();
                //直接重置计时器,在上一行已经重置了
            }

            if (f.kind == FRAME_DATA)
            {
                dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);
                if (f.seq == frame_expected)
                {
                    put_packet(f.data, len - 7);
                    //将数据放到网络层
                    frame_expected = 1 - frame_expected;
                    start_ack_timer(ACK_TIMER);
                    start_ack_time = true;
                    // send_ack_frame(); //不会被送到网络层，但是会导致最后一个收到的数据帧的确认备重复发过去
                    //这里是接到了受损帧和重复帧都处理方式。
                    //首先我从len<5 这里知道了有问题，但是不是在哪里处理的
                }
                else
                {
                    frame_expected = 1 - frame_expected;
                    send_nak_frame();
                }
            }
            if (f.ack == frame_nr)
            {
                //ack是我刚刚发过去的帧的确认，就是说我重传过去的帧备确认成功了。不一定是重传过去的，是所有的，
                stop_timer(frame_nr); //所以暂停这个计时器
                nbuffered--;          //缓冲总数减少一位
                frame_nr = 1 - frame_nr;
            }
            break;

        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            send_data_frame();
            break;
        case ACK_TIMEOUT:
            dbg_event("---- ACK %d timeout\n", arg);
            send_ack_frame();
            start_ack_time = false;
            //ACK定时器超时了
        }

        if (nbuffered < 1 && phl_ready) //缓冲区是空的，允许从网络层拿到新的数据
            enable_network_layer();
        else
            disable_network_layer(); //没有收到ACK？
    }
}