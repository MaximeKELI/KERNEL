#ifndef SKBUFF_H
#define SKBUFF_H

#include "types.h"
#include "net.h"

/* Socket buffer */
typedef struct sk_buff {
    struct sk_buff* next;
    struct sk_buff* prev;
    
    u8* head;          /* Start of buffer */
    u8* data;          /* Start of data */
    u8* tail;          /* End of data */
    u8* end;           /* End of buffer */
    
    u32 len;           /* Length of data */
    u32 data_len;      /* Data length in fragments */
    u32 truesize;      /* Total size */
    
    u16 protocol;      /* Protocol */
    u16 pkt_type;      /* Packet type */
    
    netif_t* dev;      /* Network device */
    
    u32 users;         /* Reference count */
    
    /* Network layer headers */
    ip_header_t* ip_hdr;  /* IP header pointer */
    
    void* private_data;
} sk_buff_t;

/* Initialize socket buffer subsystem */
void skb_init(void);

/* Allocate/free socket buffer */
sk_buff_t* skb_alloc(size_t size);
void skb_free(sk_buff_t* skb);

/* Manipulate socket buffer */
void* skb_put(sk_buff_t* skb, size_t len);
void* skb_push(sk_buff_t* skb, size_t len);
void* skb_pull(sk_buff_t* skb, size_t len);
void skb_reserve(sk_buff_t* skb, size_t len);

/* Clone socket buffer */
sk_buff_t* skb_clone(sk_buff_t* skb);

#endif /* SKBUFF_H */
