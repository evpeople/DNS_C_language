#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//遇见的问题，fileB4是所有的问题，大概就是丢了ACK之后，会导致原本的发送方一直发送实际上已经到达的消息，导致死锁，
#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 2000
#define BUFFERS_NUM 11
#define RECEVE_NUM 6
#define SEND_NUM 6
static unsigned char send_buffers[BUFFERS_NUM + 1][PKT_LEN + 1];
static unsigned char rece_buffers[BUFFERS_NUM + 1][PKT_LEN];
static unsigned char number_of_send;
static unsigned char frame_expected;
static unsigned char too_far = 2;
static unsigned char have_nak[BUFFERS_NUM + 1];
static unsigned char have_ack[BUFFERS_NUM + 1];
static unsigned char have_arrived[BUFFERS_NUM + 1];          //我发送的，已经收到了ACK
static unsigned char have_arrived_for_send[BUFFERS_NUM + 1]; //对面发送的，没有放到网络层

static unsigned char have_send_but_not_ack[BUFFERS_NUM + 1];
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
static struct FRAME to_phls[BUFFERS_NUM + 1];
static void put_frame(unsigned char *frame, int len);
static void send_data_frame(unsigned char);
static void send_ack_frame(unsigned char);
static void chang_number(unsigned char *);
static bool in_middle(unsigned char, unsigned char, unsigned char);
static void send_nak_frame(unsigned char to_send_nak);
static int minus(int);
static void put_frame_in_buffers(unsigned char *packet, int seq);
static bool buffers_ordered();
int main(int argc, char **argv)
{
    int event, arg;
    int minACK = 0;
    int haveACK = 0;
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
    have_send_but_not_ack[0] = 0;
    for (;;)
    {
        dbg_error("numberOFSEND is %d\n", number_of_send);
        dbg_warning("event before is %d\n", event);
        event = wait_for_event(&arg); //arg 是超时的计时器的编号
        dbg_warning("event is %d\n", event);
        int xx = 0;
        switch (event)
        {
        case NETWORK_LAYER_READY: //可以发送新的数据了（可以从网络层拿包了
            // dbg_warning("进入了网络层准备好、\n");
            get_packet(send_buffers[number_of_send]);
            have_ack[number_of_send] = 1;
            have_send_but_not_ack[1] = number_of_send;
            // have_arrived[number_of_send] = 0;
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
                    if (have_nak[frame_expected] == 0)
                    {
                        dbg_frame("发送了NAK帧，帧是%d \n", frame_expected);
                        send_nak_frame(frame_expected);
                        have_nak[frame_expected] = 1;
                    }
                }
                else
                {
                    frame_ok = true;
                }
                // if (to_network.seq == 1)
                // {
                //     memcpy(&to_phl, &to_network, sizeof(to_network));
                // }

                dbg_frame("预备了缓冲区,%d 缓冲区已有数量%d \n", in_middle(frame_expected, to_network.seq, too_far), have_arrived[BUFFERS_NUM + 1]);

                dbg_frame("frame_expected %d tonet_work.seq %d  toofar %d \n", frame_expected, to_network.seq, too_far);
                if (frame_ok)
                {
                    dbg_frame("放入了网络层\n");
                    // dbg_frame("*************%s\n", to_network.data);
                    put_packet(to_network.data, len - 7);
                    have_arrived_for_send[to_network.seq] = 0; //对面发送的，已经放到了网络层
                    //此处先不考虑捎带确认。
                    //frame_expected++;
                    int temp = frame_expected;

                    inc(temp);
                    dbg_frame("%d  have_arr[temp] %d \n", have_arrived[temp], temp);
                    while (1)
                    {
                        dbg_frame("进入了while循环\n");

                        if (have_arrived_for_send[temp] == 1) //还没放进了网络层
                        {
                            // if ((frame_expected + 1) == 1)
                            // {
                            //     put_packet(to_phl.data, 256);
                            //     dbg_frame("赋值帧成功\n");
                            // }

                            // dbg_frame("%s \n", send_buffers[frame_expected + 1]);
                            put_packet(to_phls[temp].data, 256);
                            have_arrived_for_send[temp] = 0; // 放进了网络层。
                            dbg_frame("在 while 循环  放入了 网络层\n");
                            have_nak[temp] = 0;
                            inc(frame_expected);
                            inc(temp);
                        }
                        else
                        {
                            break;
                        }
                    }
                    send_ack_frame(minus(temp));
                    have_nak[temp] = 0;
                    inc(frame_expected);
                    inc(too_far);
                    // send_ack_frame(to_network.seq);
                }

                else if (in_middle(frame_expected, to_network.seq, too_far) && have_arrived[BUFFERS_NUM + 1] != BUFFERS_NUM)
                {
                    memcpy(&to_phls[to_network.seq], &to_network, sizeof(to_network));
                    inc(too_far);
                    have_arrived[to_network.seq] == 1;
                    have_arrived[BUFFERS_NUM + 1]++;
                    have_arrived_for_send[to_network.seq] = 1; //被缓冲，还没放进网络层。
                    dbg_frame("放入了缓冲区,%d 缓冲区已有数量%d \n", to_network.seq, have_arrived[BUFFERS_NUM + 1]);
                    // send_ack_frame(to_network.seq);
                    // while (1)
                    // {
                    //     if (have_arrived[frame_expected] == 1)
                    //     {
                    //         put_packet(rece_buffers[frame_expected], 256);
                    //         frame_expected++;
                    //     }
                    //     else
                    //     {
                    //         break;
                    //     }
                    // }
                }
                // else if (have_arrived[to_network.seq])
                // {
                //
                //     send_nak_frame(frame_expected);
                //     //todo
                // }
            }
            if (to_network.kind == FRAME_ACK)
            {
                dbg_error("***************************************收到ACK后的 numOFSEND*************%d\n", number_of_send);
                dbg_event("收到了  ACK  帧， ACK  号是 %d not_receive 是 %d  number_OF_SEND  %d \n", to_network.ack, not_recvive, number_of_send);

                // minACK = minACK < to_network.ack ? minACK : to_network.ack;

                for (size_t i = minACK; i != to_network.ack;)
                {
                    if (have_ack[i] == 1)
                    {
                        stop_timer(i);
                        have_ack[i] = 0;
                        have_arrived[i] = 1;
                        dbg_event("停止了计时器%d \n", i);
                    }
                    inc(i);
                }
                minACK = to_network.ack;
                stop_timer(to_network.ack);
                have_ack[to_network.ack] = 0;
                have_arrived[to_network.ack] = 1;
                dbg_event("停止了计时器%d \n", to_network.ack);
                if (have_send_but_not_ack[0] == to_network.ack)
                {
                    have_send_but_not_ack[0]++;
                }
                else if (to_network.ack > have_send_but_not_ack[0])
                {
                    have_send_but_not_ack[0] = to_network.ack;
                }

                // if (in_middle(to_network.ack, not_recvive, number_of_send)) //大于就算成功，能大于说明之前也得到了
                // {
                //     dbg_frame("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~测试in_middle 成功\n");
                //     dbg_error("#######################到have_ack[not_recvive] = 1;后的 numOFSEND*************%d\n", number_of_send);
                //     have_ack[not_recvive] = 0;
                //     // for (size_t i = (not_recvive + 1) % BUFFERS_NUM; i <= to_network.ack; i++)
                //     // {
                //     //     have_ack[i] = 1;
                //     // }
                //     not_recvive = to_network.ack;
                //     dbg_error("#######################到 have_ack[1 + BUFFERS_NUM]++;后的 numOFSEND*************%d\n", number_of_send);
                //     have_ack[1 + BUFFERS_NUM]++;
                //     dbg_error("#######################到后stop_timer(not_recvive);的 numOFSEND*************%d\n", number_of_send);
                //     stop_timer(not_recvive);
                //     number_of_send = minus(number_of_send);
                //     dbg_error("#######################到后的not_recvive++; numOFSEND*************%d\n", number_of_send);
                //     not_recvive++;
                //     dbg_error("#######################到后chang_number(&not_recvive);的 numOFSEND*************%d\n", number_of_send);
                //     chang_number(&not_recvive);
                //     dbg_error("#######################到ACK后的 numOFSEND*************%d\n", number_of_send);
                // }
                dbg_error("处理ACK后的 numOFSEND %d\n", number_of_send);
            }
            if (to_network.kind == FRAME_NAK)
            {
                dbg_frame("对NAK的反应，重发的是%d\n", to_network.ack);
                send_data_frame((to_network.ack + 1) % (BUFFERS_NUM + 1));
            }

            dbg_error("完成获取数据后的 numOFSEND %d\n", number_of_send);
            break;
        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            for (size_t i = 0; i <= BUFFERS_NUM; i++)
            {

                if (have_ack[i])
                {
                    // dbg_frame("have_ack = %d\n", have_ack[i]);
                    dbg_frame("被重发的都有%d \n", i);
                    send_data_frame(i);
                }
            }
            break;
            dbg_error("完成超时后的 numOFSEND %d\n", number_of_send);
        case ACK_TIMEOUT:
            break;
        }
        dbg_error("完成swich后的 numOFSEND %d\n", number_of_send);
        dbg_warning("完成了switch,number_of_send 是%d \n", number_of_send);
        //物理层准备好，缓冲没有满，待发送的位置的ack已经到了
        if (phl_ready && (have_ack[(number_of_send)] == 0))
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
static void put_frame_in_buffers(unsigned char *packet, int seq)
{
    memcpy(send_buffers[seq], packet, PKT_LEN + 1);
    dbg_frame("put %s to %s \n", packet, send_buffers[seq]);
    dbg_frame("将帧 %d 放入了缓冲区\n", seq);
}
