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
#include "../minitcpip_stack.h"
#define SERV_PORT 5576
using namespace std;

ssize_t n;
struct sockaddr_in servaddr;
int sockfd;

struct sockaddr_in servaddr1;
int sockfd1;

int main(int argc,char** argv)
{
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(SERV_PORT);
    inet_pton(AF_INET,argv[1],&servaddr.sin_addr);
    connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));



    //Physical Connection Established:nclient.cpp
    //###TCP/IP SIMULATION START
    string line;
    int i;
    int tcp_dataLen;
    tcpH syncHeader;
    tcpH ackHeader;
    tcpH finHeader;
    tcpH dataHeader;
    frame syncframe;
    frame ackframe;
    frame dataframe;
    frame finframe;
    int sync_ackSeq[32];
    int data_ackSeq[32];
    int fin_ackSeq[32];
    for(i=0;i<32;i++)
    {
        data_ackSeq[i]=0;
        sync_ackSeq[i]=0;
        fin_ackSeq[i]=0;
    }
    sync_ackSeq[23]=1;//as per slide;supposed to be random
    data_ackSeq[23]=1;data_ackSeq[31]=1;//some number;supposed to be random
    fin_ackSeq[14]=1;//some number;supposed to be random

    //socket_read to "TCP_Sync(fromSender).txt"
    cout<<"\n**********READING SYNCHRONIZATION************\n";
    recieve_socket(sockfd, (char*)"MACSync(fromSender).txt",0);
    syncframe=mac_read((char*)"MACSync(fromSender).txt");
    detachFrame(syncframe,(char*)"NETIPsync(fromSender).txt");
    netLayer_Read((char*)"NETIPsync(fromSender).txt",(char*)"TCP_Sync(fromSender).txt",0);
    syncHeader=tcpRead((char*)"TCP_Sync(fromSender).txt",0);
    cout<<"\n**********SYNCHRONIZATION RECIEVED************\n";

    cout<<"\n********SENDING SYNC_ACK**********\n";
    transLayer_sendSync_Ack(sync_ackSeq, syncHeader.sequenceNumber,sockfd,(char*)"Tsync_ack.txt");
    netLayer_addIPH((char*)"Tsync_ack.txt",(char*)"Nsync_ack.txt");
    mac_buildFrame((char*)"Nsync_ack.txt",(char*)"MACsync_ack.txt");
    send_socket(sockfd,(char*)"MACsync_ack.txt");
    cout<<"\n********SYNC_ACK SENT**********\n";


    cout<<"\n**********READING ACKNOWLEDGEMENT************\n";
    recieve_socket(sockfd, (char*)"MACack(fromSender).txt",0);
    ackframe=mac_read((char*)"MACack(fromSender).txt");
    detachFrame(ackframe,(char*)"NETIPack(fromSender).txt");
    netLayer_Read((char*)"NETIPack(fromSender).txt",(char*)"TCP_ack(fromSender).txt",0);
    ackHeader=tcpRead((char*)"TCP_ack(fromSender).txt",0);
    cout<<"\n**********ACKNOWLEDGEMENT RECIEVED************\n";
    //socket_read to "TCP_ACK(fromSender).txt"

    //socket_read to "TCP_DATA(fromSender).txt"
    //initisalize tcp_dataLen while reading the 6th line from tcpH to "TCP_DATA(fromSender).txt" till EOF
    cout<<"\n#############WAITING FOR DATA###############\n";
    recieve_socket(sockfd, (char*)"MAC_DATA(fromSender).txt",1);
    dataframe=mac_read((char*)"MAC_DATA(fromSender).txt");
    detachFrame(dataframe,(char*)"NETIP_DATA(fromSender).txt");
    netLayer_Read((char*)"NETIP_DATA(fromSender).txt",(char*)"TCP_DATA(fromSender).txt",1);
    dataHeader=tcpRead((char*)"TCP_DATA(fromSender).txt",1);
    cout<<"\n##############DATA RECIEVED#################\n";

    cout<<"dataLength= "<<dataHeader.n<<endl;
    tcpWrite((char*)"checkingDataHeader.txt", dataHeader,dataHeader.n);
    cout<<"writing dataHeader.data[] to FTP_DATA(fromSender)\n";
    detachTcpH(dataHeader,(char*)"FTP_DATA(fromSender).txt");
    detatch_ftpH((char*)"FTP_DATA(fromSender).txt",(char*)"AO(fromSender).txt");
    //data decode
    /*
    if (recivedFtpH_file.is_open())
    {
        for(i=0;i<tcp_dataLen;i++)
        {
            recivedFtpH_file <<dataHeader.data[i];
        }
        recivedFtpH_file.close();
    }
    else
        cout << "Unable to open file";
    cout<<"sent to Application layer\n**************\n";
    //Application Layer decode: appLayer_removeFtpHeader("FTP_Data(fromSender)");

    //looking for finish msg
    //socket_read to "TCP_Fin(fromSender).txt"
/*
    finHeader=tcpRead("TCP_Fin(fromSender).txt",0);
    //finishAck
    transLayer_sendAck(finSeq, finHeader.sequenceNumber);

    //*/
    //###TCP/IP SIMULATION END
    //Closing Physical Connection
    /*
    close(sockfd);

    sockfd1=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr1,sizeof(servaddr1));
    servaddr1.sin_family=AF_INET;
    servaddr1.sin_port=htons(SERV_PORT);
    inet_pton(AF_INET,argv[1],&servaddr1.sin_addr);
    connect(sockfd1,(struct sockaddr*)&servaddr1,sizeof(servaddr1));
    */
    //sleep(3);
    /*
    cout<<"\n********SENDING Data_ACK**********\n";
    transLayer_sendSync_Ack(data_ackSeq, syncHeader.sequenceNumber,sockfd,(char*)"Tdata_ack.txt");
    netLayer_addIPH((char*)"Tdata_ack.txt",(char*)"Ndata_ack.txt");
    mac_buildFrame((char*)"Ndata_ack.txt",(char*)"MACdata_ack.txt");
    send_socket(sockfd,(char*)"MACdata_ack.txt");
    cout<<"\n********Data_ACK SENT**********\n";
    */
    char dataackmsg[15]="data reached";
    write(sockfd,dataackmsg,sizeof(dataackmsg));

    cout<<"\n**********READING FINISH************\n";
    recieve_socket(sockfd, (char*)"MACfin(fromSender).txt",0);
    finframe=mac_read((char*)"MACfin(fromSender).txt");
    detachFrame(finframe,(char*)"NETIPfin(fromSender).txt");
    netLayer_Read((char*)"NETIPfin(fromSender).txt",(char*)"TCP_fin(fromSender).txt",0);
    finHeader=tcpRead((char*)"TCP_fin(fromSender).txt",0);
    cout<<"\n**********FINISH RECIEVED************\n";

    cout<<"\n********SENDING FIN_ACK**********\n";
    transLayer_sendAck(fin_ackSeq, finHeader.sequenceNumber,sockfd,(char*)"Tfin_ack.txt");
    netLayer_addIPH((char*)"Tfin_ack.txt",(char*)"Nfin_ack.txt");
    mac_buildFrame((char*)"Nfin_ack.txt",(char*)"MACfin_ack.txt");
    send_socket(sockfd,(char*)"MACfin_ack.txt");
    cout<<"\n********FIN_ACK SENT**********\n";

    //close(sockfd1);
    //*/
    close(sockfd);
    return 0;
}
