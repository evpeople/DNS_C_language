#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"

#define MAX_SEQ 31
#define DATA_TIMER 3800
#define ACK_TIMER 1100
#define NR_BUFS ((MAX_SEQ + 1) / 2) //保证没有重叠，否则要很复杂的处理的重传帧的问题
#define inc(k)       \
    if (k < MAX_SEQ) \
        k++;         \
    else             \
        k = 0

static int phl_ready = 0;
bool no_nak = true;                // 避免多次发送NAK，如果不做这个处理，会导致NAK多次发送，同时发送方就会多次重传此帧
static char have_arrived[NR_BUFS]; //初始化为0
unsigned char to_phy_buffer[NR_BUFS][PKT_LEN];
unsigned char to_network_buffer[NR_BUFS][PKT_LEN];

struct FRAME
{
    unsigned char kind;
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding; //填充字段
};
static int between(unsigned char a, unsigned char b, unsigned char c); //
static void put_frame(unsigned char *frame, int len);
static void send_data(unsigned char kind_of_frame, unsigned char frame_nr, unsigned char frame_expected);
int main(int argc, char **argv)
{

    int event, arg; // arg 超时计时器的编号
    int len = 0;
    struct FRAME r;                       //待发的帧
    unsigned char next_frame_to_send = 0; //发送窗口上界，在没有ACK的时候，我应该不断往前发送，直到发送窗口被填满
    unsigned char ack_expected = 0;       //发送窗口的下界，收到ACK则代表之前的帧全部被确认了
    unsigned char frame_expected = 0;     //接受窗口的下界，接收到这个，就直接塞到网络层，然后整个窗口平移
    unsigned char too_far = NR_BUFS;      //接受窗口的上界，不能等于总大小，分不清新帧和旧帧了

    //发送是慢慢长的，接受是一开始就最大
    unsigned char now_postion = 0; //新来的帧将会在的位置

    //不用全局变量是为了
    enable_network_layer();
    //disable_network_layer();//allow netword_layer_ready events
    protocol_init(argc, argv);
    lprintf("Designed by Jiang Yanjun, build: " __DATE__ "  "__TIME__
            "\n");
    while (true)
    {
        event = wait_for_event(&arg);
        switch (event)
        {
        case NETWORK_LAYER_READY:
            dbg_warning("进入了网络层准备好、\n");
            get_packet(to_phy_buffer[next_frame_to_send % NR_BUFS]);   //虽然在扩大发送口的时候，已经用了会归零的INC，但是此处仍要取模，为的是将其二分
            send_data(FRAME_DATA, next_frame_to_send, frame_expected); //发包
            inc(next_frame_to_send);                                   //扩大发送窗口
            now_postion++;
            break;
        case PHYSICAL_LAYER_READY:
            dbg_warning("进入了物理层准备好、\n");
            phl_ready = 1;
            break;
        case FRAME_RECEIVED:
            dbg_event("接收到了帧、\n");
            len = recv_frame((unsigned char *)&r, sizeof r); //？？可能是计算帧长？
            if (len < 5 || crc32((unsigned char *)&r, len) != 0)
            { //CRC error
                dbg_event("CRC 校验出错！\n");
                if (no_nak) //第一次NAK
                {
                    send_data(FRAME_NAK, 0, frame_expected);
                }
                break;
            }
            if (r.kind == FRAME_DATA)
            {
                dbg_frame("Recv DATA %d %d, ID %d  frame_expect is %d\n", r.seq, r.ack, *(short *)r.data, frame_expected);
                if ((r.seq != frame_expected) && no_nak) //还没发送过NAK
                {                                        //只要不是我想要的，都该发送NAK,只是有些可以存，有些缓存都不行
                    dbg_frame("发送了NAK");
                    send_data(FRAME_NAK, 0, frame_expected);
                }
                else
                {
                    dbg_frame("启动了ACK计时器");
                    start_ack_timer(ACK_TIMER); //捎带确认
                }
                if (between(frame_expected, r.seq, too_far) && (have_arrived[r.seq % NR_BUFS] == false))
                {                                         //在发送窗口，但是还没收到过
                    have_arrived[r.seq % NR_BUFS] = true; //

                    memcpy(to_network_buffer[r.seq % NR_BUFS], r.data, sizeof(r.data));
                    //只用缓存数据
                    //检测是否排序好了，只要首个排好了，后面直接加就行，直到一个不可以的
                    while (have_arrived[frame_expected % NR_BUFS])
                    {
                        put_packet(to_network_buffer[frame_expected % NR_BUFS], len - 7);
                        no_nak = true; //已经是对以后的帧而言了
                        have_arrived[frame_expected % NR_BUFS] = false;
                        inc(frame_expected);
                        inc(too_far);
                        start_ack_timer(ACK_TIMER); //按照之前的ACK超时，
                        //就是说，对于正确该放到网络层的和不该放到网络层的，都在一起处理，所以需要一个ACK就可，这个的作用是，限定时间没捎带确认，就自己确认
                    }
                }
            }
            if ((r.kind == FRAME_NAK) && between(ack_expected, (r.ack + 1) % (MAX_SEQ + 1), next_frame_to_send))
            {
                send_data(FRAME_DATA, (r.ack + 1) % (MAX_SEQ + 1), frame_expected);
            }
            while (between(ack_expected, r.ack, next_frame_to_send))
            { //收到一个ACK，就把之前的全部确认了
                now_postion--;
                stop_timer(ack_expected % NR_BUFS);
                inc(ack_expected);
            }
            break;
        case ACK_TIMEOUT:
            dbg_event("---- DATA的ACK %d 超时了\n", arg);
            send_data(FRAME_ACK, 0, frame_expected); //发送的是，最后接受的那个，就是塞到网络层的那个，即便后面还收到了一些，也不发ACK，避免其实丢了一些
            dbg_frame("ACK 发送\n");
            break;
        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            if (!between(ack_expected, arg, next_frame_to_send))
                arg = arg + NR_BUFS;
            send_data(FRAME_DATA, arg, frame_expected); //select the bad frame and resend it
            break;
        }
        if (now_postion < NR_BUFS && phl_ready)
        {
            enable_network_layer();
        }
        else
        {
            disable_network_layer();
        }
    }
}
static int between(unsigned char a, unsigned char b, unsigned char c)
{
    if ((a <= b && b < c) || (c < a && a <= b) || (b < c && c < a))
        return true;
    else
        return false;
}

static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

static void send_data(unsigned char kind_of_frame, unsigned char frame_nr, unsigned char frame_expected)
{
    struct FRAME s;
    s.kind = kind_of_frame;
    s.seq = frame_nr;                                   //insert sequence number into frame
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); //piggyback ack
    if (kind_of_frame == FRAME_DATA)
        memcpy(s.data, to_phy_buffer[frame_nr % NR_BUFS], PKT_LEN); //insert packet into frame

    if (kind_of_frame == FRAME_NAK)
        no_nak = false; //one nak per frame,please
    dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);
    if (kind_of_frame == FRAME_DATA)
        put_frame((unsigned char *)&s, 3 + PKT_LEN); //put CRC following the frame
    if (kind_of_frame == FRAME_NAK || kind_of_frame == FRAME_ACK)
        put_frame((unsigned char *)&s, 2); //put CRC following the frame
    if (kind_of_frame == FRAME_DATA)
        start_timer(frame_nr % NR_BUFS, DATA_TIMER);
    stop_ack_timer(); //no need for separate ack frame
}