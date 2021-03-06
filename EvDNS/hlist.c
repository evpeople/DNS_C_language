#include "hlist.h"
#include "log.h"
#include "net.h"

int hashCode(char *key)
{
    ////UNIX系统使用的哈希
    char *k = key;
    unsigned long h = 0;
    while (*k)
    {
        h = (h << 4) + *k++;
        unsigned long g = h & 0xF0000000L;
        if (g)
        {
            h ^= g >> 24;
        }
        h &= ~g;
    }
    return h % MAPLENGTH;
}

void createHasMap(struct hashMap **hashMap)
{

    (*hashMap) = malloc(sizeof(struct hashMap));
    memset((*hashMap)->hlist, 0, sizeof((*hashMap)->hlist));
    // for (size_t i = 0; i < MAPLENGTH; i++)
    // {
    //     hashMap->hlist[i] = NULL;
    // }

    dbg_info("2m init success \n");
}
void addHashMap(char *key, char *value, struct hashMap **hashMap) //key 是 domin， value 是ip
{
    struct domainMap *node;
    node = malloc(sizeof(struct domainMap));
    node->key = malloc(40);   //用于存域名的大小，和存数字的大小
    node->value = malloc(24); //ip的大小和socketAdder的大小
    memcpy(node->key, key, 40);
    // dbg_temp("key is %s ********** yuan key %s da xiao ww %d \n ", node->key, key, strlen(node->key));
    memcpy(node->value, value, 24);
    node->hash.next = NULL;
    // node->hash.next = NULL;
    int index = hashCode(key);

    if ((*hashMap)->hlist[index] == NULL)
    {
        (*hashMap)->hlist[index] = malloc(sizeof(struct hlistHead));
        (*hashMap)->hlist[index]->first = &(node->hash);
        node->hash.pprev = &((*hashMap)->hlist[index])->first;
    }
    else
    {
        node->hash.pprev = (*hashMap)->hlist[index]->first->pprev;
        node->hash.next = (*hashMap)->hlist[index]->first;
        (*hashMap)->hlist[index]->first->pprev = &(node->hash).next;
        *(node->hash.pprev) = &(node->hash);
    }
}
// void addCacheMap(char *key, );

void hashMapInit(struct hashMap **hashMap)
{
    // createHasMap(hashMap);
    // *hashMap = malloc(sizeof(struct hashMap));

    FILE *fp = NULL;
    char ip[IPLENGTH];
    dbg_info("befor open success \n");
    char domain[DOMAINLENTH];
    fp = fopen("/home/wangzhe/DNS/DNS/dnsrelay.txt", "a+");
    if (fp == NULL)
    {
        dbg_error("can't open file\n");
        exit(1);
    }
    dbg_info("open success \n");
    while (!feof(fp))
    {
        fscanf(fp, "%s", ip);
        fscanf(fp, "%s", domain);
        dbg_temp("ip is %s \n", ip);
        addHashMap(domain, ip, hashMap);
    }
    fclose(fp);
}

char *findHashMap(struct hashMap **hashMap, char *key, char **value)
{
    bool find = false;
    int index = hashCode(key);

    if ((*hashMap)->hlist[index] != NULL)
    {
        struct domainMap *temp = (struct domainMap *)((*hashMap)->hlist[index]->first - 1); //为内存偏移的起始地址
        while (&(temp->hash) != NULL)
        {
            if (!strcasecmp(key, temp->key))
            {
                // if (!strcmp(temp->value, "0.0.0.0"))
                // {
                //     dbg_info("屏蔽网站");
                //     return "Not have this";
                // }
                find = true;
                dbg_info("返回的ip是%s\n", temp->value);
                strcpy(*value, temp->value);
                break;
                // return temp->value;
            }
            temp = (struct domainMap *)(temp->hash.next - 1);
        }
    }
    if (!find)
    {
        dbg_info("没有匹配到\n");
        **value = 'Z';
        return NULL;
    }
}

// int main(int argc, char **argv)
// {
//     config(argc, argv);

//     lprintf("\nde\n");
//     dbg_info("befor 7 init success \n");
//     struct hashMap *hashMap;
//     dbg_info("befor 3 init success \n");
//     createHasMap(hashMap);
//     dbg_info("befor init success \n");
//     hashMapInit(hashMap);

//     int fd = initServer();
//     char *temp;
//     handleDnsMsg(fd, temp);
//     dbg_info("after handle domain is %s\n", temp);
//     char *ans = findHashMap(hashMap, temp);
//     dbg_info("got ip is %s\n", ans);
//     return 0;
// }