#ifndef _HS_LIST_H_
#define _HS_LIST_H_

struct list_head {
   struct list_head *prev;
   struct list_head *next;
};

#define offsetof(type, member) ((size_t)(&((type *)0)->member))
#define container_of(ptr, type, member) ((type *)((size_t)ptr - offsetof(type, member)))

static void INIT_LIST_HEAD(struct list_head *list)
{
    list->prev = list;
    list->next = list;
}

static void __list_add(struct list_head *list, struct list_head *prev_node, struct list_head *next_node)
{
    list->prev = prev_node;
    list->next = next_node;
    prev_node->next = list;
    next_node->prev = list;
}

static void list_add(struct list_head *list, struct list_head *head)
{
    __list_add(list, head, head->next);
}

static void list_add_tail(struct list_head *list , struct list_head *head)
{
    __list_add(list, head->prev, head);
}

static void list_del(struct list_head *list)
{
    list->prev->next = list->next;
    list->next->prev = list->prev;
 
    INIT_LIST_HEAD(list);
}

static int list_is_empty(struct list_head *list)
{
    return list->next == list;
}

#define list_for_each(cur, head) \
    for (cur = (head)->next; (cur) != (head); cur = (cur)->next)

#define list_for_each_safe(cur, tmp, head) \
    for (cur = (head)->next, tmp = (cur)->next; (cur) != (head); cur = (tmp), tmp = (tmp)->next)

#define list_for_each_entry(ptr, head, member) \
    for (ptr = container_of((head)->next, typeof(*(ptr)), member); &((ptr)->member) != (head); \
        ptr = container_of((ptr)->member.next, typeof(*(ptr)), member))

typedef struct HsList
{
    int i;
} HsList;

int hsListInit(HsList *list);
int hsListFini(HsList *list);
char *hsListPushBack(HsList *list, int size);
char *hsListFront(HsList *list, int *size);
int hsListPopFront(HsList *list);

#endif