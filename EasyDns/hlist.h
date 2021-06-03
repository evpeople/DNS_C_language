#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define IPLENGTH 20
#define DOMAINLENTH 400
#define MAPLENGTH 400

#define STATIC 1
#define DYNAMIC 2

struct hlistHead
{
    struct hlistNode *first;
};

struct hlistNode
{
    struct hlistNode *next, **pprev;
};

struct hashMap
{
    struct hlistHead *hlist[MAPLENGTH];
};

struct domainMap
{
    char *key; //domin
    // char *value; //ip
    ulong value;
    long TTL;
    long lastCallTime;
    struct hlistNode hash;
};

struct cache
{
    uint16_t key; //host id
    struct sockaddr *value;
};

void hashMapInit(struct hashMap **hashMap);
int hashCode(char *key);
void createHasMap(struct hashMap **hashMap);
int findHashMap(struct hashMap **hashMap, char *key, ulong *value);
void addHashMap(char *key, char *value, struct hashMap **hashMap, int kind, int ttl); //key 是 domin， value 是ip
