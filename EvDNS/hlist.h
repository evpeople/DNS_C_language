#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IPLENGTH 20
#define DOMAINLENTH 400
#define MAPLENGTH 400
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
    char *key;   //domin
    char *value; //ip
    struct hlistNode hash;
};

void hashMapInit(struct hashMap **hashMap);
int hashCode(char *key);
void createHasMap(struct hashMap **hashMap);
char *findHashMap(struct hashMap **hashMap, char *key, char **value);
void addHashMap(char *key, char *value, struct hashMap **hashMap); //key 是 domin， value 是ip