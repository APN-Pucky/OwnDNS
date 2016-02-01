/*
 *  DNS Server Through PHP
 *  author    APNPucky
 *  date      17.5.15
 */

//Header Files
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<stdlib.h>    //malloc
#include<sys/socket.h>    //you know what this is for
#include<arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>    //getpid
#include<netdb.h>
#include <unistd.h>
#include "util.h"
#include "util-network.h"
//List of DNS Servers registered on the system
char dns_servers[10][100];
int dns_server_count = 0;
//Types of DNS resource records :)

#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server

#define PORT 53 //DNS Port

//DNS header structure
struct DNS_HEADER
{
    unsigned short id; // identification number

    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag

    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available

    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};

//Constant sized fields of query structure
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};

//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

//Structure of a Query
typedef struct
{
    unsigned char *name;
    struct QUESTION *ques;
} QUERY;

int handle_request(unsigned char *buf,unsigned char *nbuf); // Handle web requests
void changetoDnsNameFormat (unsigned char*,unsigned char*);
unsigned char* readName (unsigned char*,unsigned char*,int*);
unsigned char* dnstoip(unsigned char* qname,unsigned char* phpip);

int main(void)
{
    unsigned char buf[65536],nbuf[65536];
    int sockfd, new_sockfd, yes = 1;
    struct sockaddr_in host_addr, client_addr;
    socklen_t sin_size;
    printf("Accepting DNS requests on port %d\n", PORT);

    if ((sockfd = socket(PF_INET , SOCK_DGRAM , IPPROTO_UDP)) == -1)
        fatal("in socket");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        fatal("setting socket option SO_REUSEADDR");

    host_addr.sin_family = AF_INET; // Host byte order
    host_addr.sin_port = htons(PORT); // Short, network byte order
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Automatically fill with my IP.
    memset(&(host_addr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
        fatal("binding to socket");

    while(1)
    {
        sin_size = sizeof(struct sockaddr_in);
        recvfrom(sockfd,buf,65536,0,(struct sockaddr *)&client_addr,&sin_size);
        
        //printf("Got a connection\n");

        pid_t pid = fork();
        if(pid ==0)
        {
	        int size = handle_request(buf,nbuf);
	        sendto(sockfd,nbuf,size,0,(struct sockaddr *)&client_addr,sin_size);
        }
    }
    return 0;
}

int handle_request(unsigned char *buf,unsigned char *nbuf)
{
    unsigned char *qname,phpip[100];
    int count=0;

    struct DNS_HEADER *dns = NULL;
    struct R_DATA *rdata = ec_malloc(sizeof(struct R_DATA));
    struct sockaddr_in si_tmp;	

    memcpy(nbuf, buf,65536);
    dns = (struct DNS_HEADER*)nbuf;
    //printf("Got a message\n");
    qname = readName((unsigned char*)&buf[sizeof(struct DNS_HEADER)],buf,&count);
    //printf("rldns: %s\n",(unsigned char*)&buf[sizeof(struct DNS_HEADER)]);
    printf("qname: %s\n",qname);
    unsigned char dnsname[strlen(qname)+1];
    memcpy(dnsname,(unsigned char*)&buf[sizeof(struct DNS_HEADER)],strlen((unsigned char*)&buf[sizeof(struct DNS_HEADER)])+1);
    //printf("dnsname: %s\n",dnsname);
    //php-pull
    //phpip!!!!

    
    dnstoip(qname,phpip);
    printf("IP: %s\n",phpip);
    if(strcmp(phpip,qname)==0){
         return 0;
    }

    dns->q_count = htons(1);
    dns->ans_count = htons(1);
    dns->rd = 1;
    dns->ra = 1;
    dns->qr = 1;
    memset(nbuf+sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+1,'\0',24);
    memset(nbuf+sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+1,(char)192,1);
    memset(nbuf+sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+1+1,(char)12,1);
    memset(nbuf+sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+1+3,(char)1,1);
    memset(nbuf+sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+1+5,(char)1,1);
    
    inet_aton(phpip,&si_tmp.sin_addr);
    
    long *p = (long *)&si_tmp.sin_addr.s_addr;

    memset(nbuf+sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+12,(char)4,1);
    memcpy(nbuf+sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+3+sizeof(struct R_DATA), p,sizeof(struct in_addr));
    return sizeof(struct DNS_HEADER)+strlen(qname)+1+sizeof(struct QUESTION)+5+sizeof(struct R_DATA)+sizeof(struct in_addr);
}

u_char* dnstoip(unsigned char* qname,unsigned char* phpip)
{
    int sockfd;
    struct sockaddr_in target_addr;
    unsigned char buffer[4096];
    
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        sprintf(phpip,"%s",qname);
        return;
    }
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(80);
    //target_addr.sin_addr = *(struct in_addr *)(((struct hostent *)gethostbyname("www.dnstoip.bplaced.net"))->h_addr);
    inet_aton("144.76.167.70",&target_addr.sin_addr);
    memset(&(target_addr.sin_zero), '\0',8);

    if(connect(sockfd, (struct sockaddr *)&target_addr, sizeof(struct sockaddr))==-1)
    {
        sprintf(phpip,"%s",qname);
        return;
    }
    unsigned char *format = "GET /index.php?dns=%s HTTP/1.1\r\nHost: dnstoip.bplaced.net\r\nConnection: keep-alive\r\n\r\n";
    unsigned char phpreq[strlen(format)+strlen(qname)];
    sprintf(phpreq,format,qname);
    send_string(sockfd,phpreq);

    while(recv_line(sockfd,buffer))
    {
         //printf(": %s\n",buffer);
    }
    recv_line(sockfd,buffer);
    recv_line(sockfd,buffer);
    sprintf(phpip,"%s",buffer);
}

u_char* readName(unsigned char* reader,unsigned char* buffer,int* count)
{
    unsigned char *name;
    unsigned int p=0,jumped=0,offset;
    int i , j;
 
    *count = 1;
    name = (unsigned char*)malloc(256);
 
    name[0]='\0';
 
    //read the names in 3www6google3com format
    while(*reader!=0)
    {
        if(*reader>=192)
        {
            offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000 ;)
            reader = buffer + offset - 1;
            jumped = 1; //we have jumped to another location so counting wont go up!
        }
        else
        {
            name[p++]=*reader;
        }
 
        reader = reader+1;
 
        if(jumped==0)
        {
            *count = *count + 1; //if we havent jumped to another location then we can count up
        }
    }
 
    name[p]='\0'; //string complete
    if(jumped==1)
    {
        *count = *count + 1; //number of steps we actually moved forward in the packet
    }
 
    //now convert 3www6google3com0 to www.google.com
    for(i=0;i<(int)strlen((const char*)name);i++) 
    {
        p=name[i];
        for(j=0;j<(int)p;j++) 
        {
            name[i]=name[i+1];
            i=i+1;
        }
        name[i]='.';
    }
    name[i-1]='\0'; //remove the last dot
    return name;
}

void changetoDnsNameFormat(unsigned char* dns,unsigned char* host) 
{
    int lock = 0 , i;
    strcat((char*)host,".");
     
    for(i = 0 ; i < strlen((char*)host) ; i++) 
    {
        if(host[i]=='.') 
        {
            *dns++ = i-lock;
            for(;lock<i;lock++) 
            {
                *dns++=host[lock];
            }
            lock++; //or lock=i+1;
        }
    }
    *dns++='\0';
}
