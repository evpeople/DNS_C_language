#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 2000

struct FRAME
{
    unsigned char kind; /* FRAME_DATA */
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};

static unsigned char frame_nr = 0, buffer[PKT_LEN], nbuffered;
static unsigned char frame_expected = 0;
static int phl_ready = 0;

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
    s.ack = 1 - frame_expected;
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

    for (;;)
    {
        event = wait_for_event(&arg);
        bool right_frame = true;
        switch (event)
        {
        case NETWORK_LAYER_READY:
            get_packet(buffer);
            nbuffered++;
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
            if (f.kind == FRAME_NAK)
            {
                dbg_frame("Recv NAK  %d\n", f.ack);
                //不出现是因为没有接受过NAK
            }

            if (f.kind == FRAME_DATA)
            {
                dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);
                if (f.seq == frame_expected)
                {
                    put_packet(f.data, len - 7);
                    frame_expected = 1 - frame_expected;
                    send_ack_frame(); //不会被送到网络层，但是会导致最后一个收到的数据帧的确认备重复发过去
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
                //ack是我刚刚发过去的帧的确认，就是说我重传过去的帧备确认成功了。
                stop_timer(frame_nr); //所以暂停这个计时器
                nbuffered--;
                frame_nr = 1 - frame_nr;
            }
            break;

        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            send_data_frame();
            break;
        }

        if (nbuffered < 1 && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
    }
}