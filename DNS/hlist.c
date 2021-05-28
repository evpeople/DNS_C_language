#include "hlist.h"
#include "log.h"
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

void createHasMap(struct hashMap *hashMap)
{
    memset(hashMap->hlist, 0, sizeof(hashMap->hlist));
}
void addHashMap(char *key, char *value, struct hashMap *hashMap) //key 是 domin， value 是ip
{
    struct domainMap *node;
    node = malloc(sizeof(struct domainMap));
    node->key = malloc(strlen(key) + 1);
    node->value = malloc(strlen(value) + 1);
    strcpy(node->key, key);
    // dbg_temp("key is %s ********** yuan key %s da xiao ww %d \n ", node->key, key, strlen(node->key));
    strcpy(node->value, value);
    node->hash.next = NULL;
    node->hash.next = NULL;
    int index = hashCode(key);

    if (hashMap->hlist[index] == NULL)
    {
        hashMap->hlist[index] = malloc(sizeof(struct hlistHead));
        hashMap->hlist[index]->first = &(node->hash);
        node->hash.pprev = &(hashMap->hlist[index])->first;
    }
    else
    {
        node->hash.pprev = hashMap->hlist[index]->first->pprev;
        node->hash.next = hashMap->hlist[index]->first;
        hashMap->hlist[index]->first->pprev = &(node->hash).next;
        *(node->hash.pprev) = &(node->hash);
    }
}

void hashMapInit(struct hashMap *hashMap)
{
    FILE *fp = NULL;
    char ip[IPLENGTH];
    char domain[DOMAINLENTH];
    fp = fopen("/home/wangzhe/DNS/DNS/dnsrelay.txt", "r");
    if (fp == NULL)
    {
        dbg_error("can't open file\n");
        exit(1);
    }
    while (!feof(fp))
    {
        fscanf(fp, "%s", ip);
        fscanf(fp, "%s", domain);
        dbg_temp("ip is %s \n", ip);
        addHashMap(domain, ip, hashMap);
    }
    fclose(fp);
}

char *findHashMap(struct hashMap *hashMap, char *key)
{
    int index = hashCode(key);
    if (hashMap->hlist[index])
    {
        struct domainMap *temp = (struct domainMap *)(hashMap->hlist[index]->first - 1); //为内存偏移的起始地址
        while (temp != NULL)
        {
            if (!strcasecmp(key, temp->key))
            {
                if (!strcmp(temp->value, "0.0.0.0"))
                {
                    dbg_info("屏蔽网站");
                    return "Not have this";
                }
                return temp->value;
            }
            temp = (struct domainMap *)(temp->hash.next - 1);
        }
    }
    else
    {
        dbg_info("没有匹配到\n");
        return NULL;
    }
}
