#include "log.h"
#include "hlist.h"
#include "net.h"

int main(int argc, char **argv)
{
    config(argc, argv);

    lprintf("\nde\n");
    dbg_info("befor 7 init success \n");
    struct hashMap *hashMap;
    dbg_info("befor 3 init success \n");
    // createHasMap(hashMap); //此处内存过多或其他问题。
    dbg_info("befor init success \n");
    hashMapInit(&hashMap);
    // findHashMap(&hashMap, "test0");

    int fd = initServer();
    char *temp = malloc(400);
    handleDnsMsg(fd, (temp));
    dbg_info("after handle domain is %s\n", (temp));
    char *ans = findHashMap(&hashMap, (temp + 1));
    dbg_info("got ip is %s\n", ans);
    return 0;
}