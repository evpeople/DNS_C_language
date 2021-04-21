#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 2000

struct FRAME
{
    unsigned char kind;
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};

static unsigned char frame_nr = 0, buffer[PKT_LEN], nbuffered;
static unsigned char frame_expected = 0;
static int phl_ready = 0;

int main(int argc, char **argv)
{
    int event, arg;
    int len = 0;
    protocol_init(argc, argv);

    lprintf("WANGZHE 's homework:"__DATE__
            " "__TIME__
            "\n");
    disable_network_layer();

    int next_frame_to_send = 0;
    int frame_expected = 0;
    struct FRAME r, s;
    get_packet(&(r.data));

    for (;;)
    {
        event = wait_for_event(&arg);

        switch (event)
        {
        case NETWORK_LAYER_READY:
            /* code */
            break;

        default:
            break;
        }
    }
}