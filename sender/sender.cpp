#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<string>
#include<fstream>
#include<cstdlib>
#include<stdlib.h>
#include<math.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include "../minitcpip_stack.h"
#define SERV_PORT 5576
using namespace std;

ssize_t n;
struct sockaddr_in servaddr,cliaddr;
int listenfd,connfd,clilen;

struct sockaddr_in servaddr1,cliaddr1;
int listenfd1,connfd1,clilen1;

int main()
{
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(SERV_PORT);
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    listen(listenfd,1);
    clilen=sizeof(cliaddr);
    connfd=accept(listenfd,(struct sockaddr*)&cliaddr,(socklen_t*)sizeof(cliaddr));//(socklen_t*)&clilen
    printf("\n client connected");



    //Physical Connection Established:nserver.cpp
    //###TCP/IP SIMULATION START
    int tcp_dataLen;
    cout<<"applayer calledNEW\n";
    tcp_dataLen=appLayer_addFtpHeader((char*)"AI.txt",(char*)"AO.txt");

    cout<<"translayer called\n";
    int i;
    tcpH sync_ackHeader;
    tcpH data_ackHeader;
    tcpH fin_ackHeader;
    frame sync_ackframe;
    frame data_ackframe;
    frame fin_ackframe;
    int syncSeq[32];
    int ackSeq[32];
    int finSeq[32];
    int sync_ackSeq[32];
    for(i=0;i<32;i++)
    {
        syncSeq[i]=0;
        ackSeq[i]=0;
        finSeq[i]=0;
    }
    syncSeq[26]=1;//as per slide;supposed to be random
    ackSeq[20]=1;//some number;supposed to be random
    finSeq[16]=1;//some number;supposed to be random
    cout<<"\n********SENDING SYNCHRONIZATION**********\n";
    transLayer_sendSync(syncSeq,connfd,(char*)"Tsync.txt");
    netLayer_addIPH((char*)"Tsync.txt",(char*)"Nsync.txt");
    mac_buildFrame((char*)"Nsync.txt",(char*)"MACsync.txt");
    send_socket(connfd,(char*)"MACsync.txt");
    cout<<"\n********SYNCHRONIZATION SENT**********\n";

    //socket_read to "TCP_Sync_Ack(fromReciever).txt"
    cout<<"\n**********READING SYNC_ACK************\n";
    recieve_socket(connfd,(char*)"MACSync_Ack(fromReciever).txt",0);
    sync_ackframe=mac_read((char*)"MACSync_Ack(fromReciever).txt");
    detachFrame(sync_ackframe,(char*)"NSync_Ack(fromReciever).txt");
    netLayer_Read((char*)"NSync_Ack(fromReciever).txt",(char*)"TCP_Sync_Ack(fromReciever).txt",0);
    sync_ackHeader=tcpRead((char*)"TCP_Sync_Ack(fromReciever).txt",0);
    cout<<"\n**********SYNCH_ACK RECIEVED************\n";


    cout<<"\n********SENDING ACKNOWLEDGEMENT**********\n";
    transLayer_sendAck(ackSeq, sync_ackHeader.sequenceNumber,connfd,(char*)"Tack.txt");
    netLayer_addIPH((char*)"Tack.txt",(char*)"Nack.txt");
    mac_buildFrame((char*)"Nack.txt",(char*)"MACack.txt");
    send_socket(connfd,(char*)"MACack.txt");
    cout<<"\n********ACKNOWLEDGEMENT SENT**********\n";


    cout<<"\n############SENDING DATA###############\n";
    transLayer_sendData(syncSeq,connfd,(char*)"TDATA.txt");
    cout<<"worked\n";
    netLayer_addIPH((char*)"TDATA.txt",(char*)"NDATA.txt");
    cout<<"workednet\n";
    mac_buildFrame((char*)"NDATA.txt",(char*)"MACDATA.txt");
    cout<<"workednmac\n";
    send_socket(connfd,(char*)"MACDATA.txt");
    cout<<"\n#############DATA SENT##################\n";
    /*
    transLayer_sendFin(finSeq);
    //socket_read to "TCP_Fin_Ack(fromReciever).txt"

    fin_ackHeader=tcpRead("TCP_Fin_Ack(fromReciever).txt",0);
    */
    //###TCP/IP SIMULATION END
    //Closing Physical Connection
    /*
    close(listenfd);

    listenfd1=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr1,sizeof(servaddr1));
    servaddr1.sin_family=AF_INET;
    servaddr1.sin_port=htons(SERV_PORT);
    bind(listenfd1,(struct sockaddr*)&servaddr1,sizeof(servaddr1));
    listen(listenfd1,1);
    clilen1=sizeof(cliaddr1);
    connfd1=accept(listenfd1,(struct sockaddr*)&cliaddr1,(socklen_t*)&clilen1);
    printf("\n client REconnected");
    */
    /*
    cout<<"\n**********READING Data_ACK************\n";
    recieve_socket(connfd,(char*)"MACdata_Ack(fromReciever).txt",0);
    data_ackframe=mac_read((char*)"MACdata_Ack(fromReciever).txt");
    detachFrame(data_ackframe,(char*)"Ndata_Ack(fromReciever).txt");
    netLayer_Read((char*)"Ndata_Ack(fromReciever).txt",(char*)"TCP_data_Ack(fromReciever).txt",0);
    data_ackHeader=tcpRead((char*)"TCP_data_Ack(fromReciever).txt",0);
    cout<<"\n**********Data_ACK RECIEVED************\n";
    */
    char dataackmsg[15];
    read(connfd,dataackmsg,15);


    cout<<"\n********SENDING FINISH**********\n";
    transLayer_sendFin(finSeq,connfd,(char*)"Tfin.txt");
    netLayer_addIPH((char*)"Tfin.txt",(char*)"Nfin.txt");
    mac_buildFrame((char*)"Nfin.txt",(char*)"MACfin.txt");
    send_socket(connfd,(char*)"MACfin.txt");
    cout<<"\n********FINISH SENT**********\n";

    cout<<"\n**********READING FIN_ACK************\n";
    recieve_socket(connfd,(char*)"MACfin_Ack(fromReciever).txt",0);
    fin_ackframe=mac_read((char*)"MACfin_Ack(fromReciever).txt");
    detachFrame(sync_ackframe,(char*)"Nfin_Ack(fromReciever).txt");
    netLayer_Read((char*)"Nfin_Ack(fromReciever).txt",(char*)"TCP_fin_Ack(fromReciever).txt",0);
    fin_ackHeader=tcpRead((char*)"TCP_fin_Ack(fromReciever).txt",0);
    cout<<"\n**********FIN_ACK RECIEVED************\n";

    //close(listenfd1);

    close(listenfd);
    return 0;
}
