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
#define SERV_PORT 5576
using namespace std;

char readBuffer[12216];

int CRC32[33]={1,0,0,0,0,0,1,0,0,1,1,0,0,0,0,0,1,0,0,0,1,1,1,0,1,1,0,1,1,0,1,1,1};
int CRC32_len=33;

int checksum[10000];

typedef struct ftpH
{
    int n;
    int desc[8];
    int byte_count[16];
    int data[524288];
}ftpH;

typedef struct tcp
{
    int n;
    int sourcePort[16];
    int destinationPort[16];
    int sequenceNumber[32];
    int ackNumber[32];
    int headerLenght[4];
    int reserved[6];
    //flags
    int urg;int ack;int psh;int rst;int syn;int fin;
    int windowSize[16];
    int checksum[16];
    int urgentPointer[16];
    int options[24];
    int padding[8];
    int data[720];
}tcpH;

typedef struct IPH
{
    int n;
    int version[4];
    int hlen[4];
    int tos[8];
    int length[16];
    int ident[16];
    int flag[16];
    int ttl[8];
    int protocol[8];
    int checksum[16];
    int srcadd[32];
    int destadd[32];
    int data[1000];
}IPH;

typedef struct frame
{
    //continuous memory allocated
    int preamble[64];
    int sfd[8];
    int sourceMac[48];
    int destinationMac[48];
    int type[16];
    int n;
    int payload[12000];//non variable size
    int crc[32];
}frame;

int fixMsgCode(char* msg,ftpH* cah);
int appLayer_addFtpHeader(char *in,char *out);
void detatch_ftpH(char *in,char *out);

tcpH tcpRead(char* rfile, int data);
void tcpWrite(char* wfile, tcpH in, int data);
void transLayer_sendSync(int* seq,int sock_id, char* Outfile);
void transLayer_sendSync_Ack(int* seq,int* syncSeq,int sock_id, char* Outfile);
void transLayer_sendAck(int* seq,int* sync_ackSeq,int sock_id, char* Outfile);//both for syncAck and finAck
void transLayer_sendFin(int* seq,int sock_id, char* Outfile);
void transLayer_sendData(int* seq,int sock_id, char* Outfile);
char* tcpCheckSumWrite(tcpH in);
void detachTcpH(tcpH dataHeader, char* FtpH_file);

IPH netLayer_Read(char *mac,char *toapp,int data);
void netLayer_write(char* str,IPH iph);
void netLayer_addIPH(char *netL,char *toMac);
void netLayer_detach(IPH ip,char *toapp);
char* getFile(IPH ip);
void getCheckSum(char* chk);

void mac_buildFrame(char* msgFile,char* outFileName);
void mac_write(frame f, char* macOutFileName);
frame mac_read(char* macInFileName);
void fixDestMac(int* destMac,frame* cf);
void fixSrcMac(int* srcMac,frame* cf);
void fixType(int* type,frame* cf);
void fixSFD(frame* cf);
void fixPreamble(frame* cf);
void fixCRC(int* msg,int msgLen,int* crcKey,int crcKeyLen,frame* cf);
int checkCrcError(frame rf, int* crcKey, int crcKeyLen);
void detachFrame(frame cf, char* IpH_file);
void printFrame(frame out);

void recieve_socket(int sock_id, char* socketIn_file, int datalines);
void send_socket(int sock_id, char* socketOut_file);

int bin2dec(char* str);
int Bctoi(char c);
int findFirstIndex(int a[],int n);
int exor(int x, int y);
int bxor(int l,int m);
int* BaddOne(int* a, int n);
void hexDisplay(int* bin, int binlen);
void hex_routine(int* hex);

void hex_routine(int* hex)
{
    int n=0,i,j=0;
    for(i=3;i>=0;i--)
    {
        n+=hex[i]*pow(2,j);
        j++;
    }
    if(n>9)
    {
        switch(n)
        {
            case 10: printf("A");break;
            case 11: printf("B");break;
            case 12: printf("C");break;
            case 13: printf("D");break;
            case 14: printf("E");break;
            case 15: printf("F");break;
        }
    }
    else
    {
        printf("%d",n);
    }
}

void hexDisplay(int* bin, int binlen)
{
    int i,j=0;
    int hex[4];
    for(i=0;i<binlen;i++)
    {
        hex[j++]=bin[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
}

int bin2dec(char* str)
{
    int sum;
     for (int i = 0; str[i] != '\0'; i++)
    {
    sum = (sum << 1) + str[i] - '0';
    }
    return sum;

}

void detatch_ftpH(char *in,char *out)
{
	int i=0,j,k,dec[100];
	char S1[9];
	string line;
	ifstream infile(in);
    ofstream myfile(out);
    if(infile.is_open()) // To get you all the lines.
    {
		getline(infile,line);
        getline(infile,line);
        getline(infile,line); // Saves the line in line.
        cout<<line<<"\n"; // Prints our line.
		infile.close();
    }
    else
	{
        cout<<"Unable to Open File :"<<in<<"\n";
    }
  	int n=line.length();
  	if(myfile.is_open())
  	{
        for(i=0;i<n;)
        {
            for(k=0;k<8;k++)
            {
                S1[k]=line[i++];
            }
            S1[8]='\0';
            //cout<<S1<<"\n";
            int p=bin2dec(S1);
            myfile<<char(p);
        }
        myfile.close();
  	}
  	else cout<<"unable to open file "<<out<<"\n";
}

int fixMsgCode(char* msg,ftpH* cah)
{
    printf("entered fixmsgcode\n");
    int i,t,j,k,r,l;
    int asc[8];
    int n=strlen(msg);
    int N=8*n;
    int MsgCode[N];
    k=0;
    for(i=0;i<n;i++)
    {
        t=msg[i];
        for(l=0;l<8;l++)
        {
            asc[l]=0;
        }
        for(j=7;j>=0;j--)
        {
            if(t>0)
            {
                r=t%2;
                asc[j]=r;
                t=t/2;
            }
        }
        for(l=0;l<8;l++)
        {
            MsgCode[k]=asc[l];
            k++;
        }
    }
    (*cah).n=N;
    for(k=0;k<N;k++)
    {
        (*cah).data[k]=MsgCode[k];
    }
    return N;
}

int appLayer_addFtpHeader(char *in,char *out)
{
    string line;
    printf("entered appFtpHeader\n");
    //cout<< AIfile;
    //ifstream apInputFile (AIfile);
    ifstream apInputFile (in);
    ofstream apOutputFile(out);
    int n,N;
    ftpH ah;
    int bc[16];
    int t,i,j,r;
    for(i=0;i<16;i++)
    {
        bc[i]=0;
    }
    if (apInputFile.is_open())
    {
        while (apInputFile.good())
        {
            char c=apInputFile.get();
            if(apInputFile.good())
                line+=c;
        }
        n=line.length();
        char msg[n];
        strcpy(msg, line.c_str());
        apInputFile.close();
        //attach data
        N=fixMsgCode(msg, &ah);
        t=N/8;
        //attaching bytecount
        for(j=15;j>=0;j--)
        {
            if(t>0)
            {
                r=t%2;
                bc[j]=r;
                t=t/2;
            }
        }
        for(i=0;i<16;i++)
        {
            ah.byte_count[i]=bc[i];
        }
        //attach descriptor
        for(i=0;i<8;i++)
        {
            ah.byte_count[i]=0;
        }
        ah.desc[1]=1;
        //writing to AO.txt
        if (apOutputFile.is_open())
        {
            //writing desc
            for(i=0;i<8;i++)
            {
                apOutputFile <<ah.desc[i];
            }
            apOutputFile << "\n";
            //writing byte_count
            for(i=0;i<16;i++)
            {
                apOutputFile <<ah.byte_count[i];
            }
            apOutputFile << "\n";
            //writing data
            for(i=0;i<N;i++)
            {
                apOutputFile <<ah.data[i];
            }
            apOutputFile << "\n";
            apOutputFile.close();
        }
        else cout << "Unable to open file";
    }
    else cout << "Unable to open file";
    return (N+24);//data length for tcpH
}

int Bctoi(char c)
{
    if(c== '0')
        return 0;
    else
        return 1;
}

tcpH tcpRead(char* rfile, int data)
{
    tcpH out;
    int i,k;
    string line,dataline;
    ifstream traInputFile(rfile);
    //cout<<"********TCP_READ*******\n";
    if (traInputFile.is_open())
    {
        getline (traInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<16;i++)
        {
            out.sourcePort[i]=Bctoi(line[k++]);
        }
        for(i=0;i<16;i++)
        {
            out.destinationPort[i]=Bctoi(line[k++]);
        }
        getline (traInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<32;i++)
        {
            out.sequenceNumber[i]=Bctoi(line[k++]);
        }
        getline (traInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<32;i++)
        {
            out.ackNumber[i]=Bctoi(line[k++]);
        }
        getline (traInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<4;i++)
        {
            out.headerLenght[i]=Bctoi(line[k++]);
        }
        for(i=0;i<6;i++)
        {
            out.reserved[i]=Bctoi(line[k++]);
        }
        out.urg=Bctoi(line[k++]);
        out.ack=Bctoi(line[k++]);
        out.psh=Bctoi(line[k++]);
        out.rst=Bctoi(line[k++]);
        out.syn=Bctoi(line[k++]);
        out.fin=Bctoi(line[k++]);
        for(i=0;i<16;i++)
        {
            out.windowSize[i]=Bctoi(line[k++]);
        }
        getline (traInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<16;i++)
        {
            out.checksum[i]=Bctoi(line[k++]);
        }
        for(i=0;i<16;i++)
        {
            out.urgentPointer[i]=Bctoi(line[k++]);
        }
        getline (traInputFile,dataline);k=0;//cout<<dataline<<"\n"<<"data came\n";
        k=dataline.length();
        out.n=k;
        if(data != 0)
        {
            for(i=0;i<k;i++)
            {
                out.data[i]=Bctoi(dataline[i]);
            }
        }
        traInputFile.close();
    }
    else cout << "Unable to open file:tcp_read\n";
    return out;
}

void tcpWrite(char* wfile, tcpH in, int data)
{
    ofstream traOutputFile(wfile);
    int i;
    //int r;
    //writing to AO.txt
    if (traOutputFile.is_open())
    {
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.sourcePort[i];
        }
        //traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.destinationPort[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<32;i++)
        {
            traOutputFile<< in.sequenceNumber[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<32;i++)
        {
            traOutputFile<< in.ackNumber[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<4;i++)
        {
            traOutputFile<< in.headerLenght[i];
        }
        for(i=0;i<6;i++)
        {
            traOutputFile<< in.reserved[i];
        }
        //flags
        traOutputFile<<in.urg;
        traOutputFile<<in.ack;
        traOutputFile<<in.psh;
        traOutputFile<<in.rst;
        traOutputFile<<in.syn;
        traOutputFile<<in.fin;
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.windowSize[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.checksum[i];
        }
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.urgentPointer[i];
        }
        traOutputFile<<"\n";
        data=in.n;
        //r=data%32;
        if(data != 0)
        {
            for(i=0;i<data;i++)
            {
                traOutputFile<< in.data[i];
            }
            traOutputFile<<"\n";
        }
        traOutputFile.close();
    }
    else cout << "Unable to open file:tcp_write\n";
}

char* tcpCheckSumWrite(tcpH in)
{
    ofstream traOutputFile("CheckSumAuxFile.txt");
    int i,r;
    int data=in.n;
    //writing to AO.txt
    if (traOutputFile.is_open())
    {
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.sourcePort[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.destinationPort[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<32;i++)
        {
            traOutputFile<< in.sequenceNumber[i];
            if(i==15)
                traOutputFile<<"\n";
        }
        traOutputFile<<"\n";
        for(i=0;i<32;i++)
        {
            traOutputFile<< in.ackNumber[i];
            if(i==15)
                traOutputFile<<"\n";
        }
        traOutputFile<<"\n";
        for(i=0;i<4;i++)
        {
            traOutputFile<< in.headerLenght[i];
        }
        for(i=0;i<6;i++)
        {
            traOutputFile<< in.reserved[i];
        }
        //flags
        traOutputFile<<in.urg;
        traOutputFile<<in.ack;
        traOutputFile<<in.psh;
        traOutputFile<<in.rst;
        traOutputFile<<in.syn;
        traOutputFile<<in.fin;
        traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.windowSize[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.checksum[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< in.urgentPointer[i];
        }
        traOutputFile<<"\n";
        if(data != 0)
        {
            r=data%16;
            for(i=0;i<data;i++)
            {
                if(i%16==0)
                    traOutputFile<<"\n";
                traOutputFile<< in.data[i];
            }
            for(i=0;i<r;i++)
            {
                traOutputFile<< "0";
            }
            traOutputFile<<"\n";
        }
        traOutputFile.close();
    }
    else cout << "Unable to open file-CheckSumAuxFile.txt\n";

    return (char*)"CheckSumAuxFile.txt";
}

void attach_tcpChecksum(tcpH* th)
{
    int i;
    char* checksum16_file;
    int csksum[16]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,};
    //attaching zeros for checksum
    for(i=0;i<16;i++)
    {
        th->checksum[i]=csksum[i];
    }
    checksum16_file=tcpCheckSumWrite(*th);
    getCheckSum(checksum16_file);
    //attaching proper checksum
    for(i=0;i<16;i++)
    {
        th->checksum[i]=checksum[i];
    }
}

int* BaddOne(int* a, int n)
{
    int i;
    for(i=n-1;i>=0;i--)
    {
        if(a[i] == 0)
        {
            a[i]=1;
            break;
        }
        else
        {
            a[i]=0;
        }
    }
    return a;
}

void transLayer_sendSync(int* syncSeq,int sock_id, char* outfile)
{
    string line;
    int i;
    printf("entered transLayer_sendSync\n");
    tcpH sync;
    //preparing sync header
    int ftpPort[16]={0,0,0,0, 0,0,0,0, 0,0,0,1, 0,1,1,0};
    for(i=0;i<16;i++)
    {
        sync.sourcePort[i]=ftpPort[i];
        sync.destinationPort[i]=ftpPort[i];
        sync.urgentPointer[i]=0;
    }
    for(i=0;i<32;i++)
    {
        sync.sequenceNumber[i]=syncSeq[i];
        sync.ackNumber[i]=0;
        sync.windowSize[i]=0;
    }
    int hLen[4]={0,1,0,1};
    for(i=0;i<4;i++)
    {
        sync.headerLenght[i]=hLen[i];
    }

    for(i=0;i<6;i++)
    {
        sync.reserved[i]=0;
    }
    sync.urg=0;
    sync.ack=0;
    sync.psh=0;
    sync.rst=0;
    sync.syn=1;
    sync.fin=0;
    sync.windowSize[4]=1;//all other were zero's; 2048
    //checksum remaining
    sync.n=0;
    attach_tcpChecksum(&sync);
    tcpWrite(outfile,sync,0);
    //send_socket(sock_id,outfile);

}

void transLayer_sendSync_Ack(int* seq, int* prevSeq,int sock_id, char* outfile)
{
    string line;
    int i;
    printf("entered transLayer_sendSync_Ack\n");
    tcpH sync_ack;
    //preparing sync_ack header
    int ftpPort[16]={0,0,0,0, 0,0,0,0, 0,0,0,1, 0,1,1,0};
    for(i=0;i<16;i++)
    {
        sync_ack.sourcePort[i]=ftpPort[i];
        sync_ack.destinationPort[i]=ftpPort[i];
        sync_ack.urgentPointer[i]=0;
    }
    int* b=BaddOne(prevSeq,32);
    for(i=0;i<32;i++)
    {
        sync_ack.sequenceNumber[i]=seq[i];
        sync_ack.ackNumber[i]=b[i];
        sync_ack.windowSize[i]=0;
    }
    int hLen[4]={0,1,0,1};
    for(i=0;i<4;i++)
    {
        sync_ack.headerLenght[i]=hLen[i];
    }

    for(i=0;i<6;i++)
    {
        sync_ack.reserved[i]=0;
    }
    sync_ack.urg=0;
    sync_ack.ack=1;
    sync_ack.psh=0;
    sync_ack.rst=0;
    sync_ack.syn=1;
    sync_ack.fin=0;
    sync_ack.windowSize[4]=1;//all other were zero's; 2048
    //checksum remaining
    sync_ack.n=0;
    attach_tcpChecksum(&sync_ack);
    tcpWrite(outfile,sync_ack,0);
    //send_socket(sock_id,outfile);
}

void transLayer_sendAck(int* ackSeq, int* prevSeq,int sock_id, char* outfile)
{
    string line;
    int i;
    printf("entered transLayer_sendAck\n");
    tcpH ack;
    //preparing ack header
    int ftpPort[16]={0,0,0,0, 0,0,0,0, 0,0,0,1, 0,1,1,0};
    for(i=0;i<16;i++)
    {
        ack.sourcePort[i]=ftpPort[i];
        ack.destinationPort[i]=ftpPort[i];
        ack.urgentPointer[i]=0;
    }
    int* b=BaddOne(prevSeq,32);
    for(i=0;i<32;i++)
    {
        ack.sequenceNumber[i]=ackSeq[i];
        ack.ackNumber[i]=b[i];
        ack.windowSize[i]=0;
    }
    int hLen[4]={0,1,0,1};
    for(i=0;i<4;i++)
    {
        ack.headerLenght[i]=hLen[i];
    }

    for(i=0;i<6;i++)
    {
        ack.reserved[i]=0;
    }
    ack.urg=0;
    ack.ack=1;
    ack.psh=0;
    ack.rst=0;
    ack.syn=0;
    ack.fin=0;
    ack.windowSize[4]=1;//all other were zero's; 2048
    //checksum remaining
    ack.n=0;
    attach_tcpChecksum(&ack);
    tcpWrite(outfile,ack,0);
    //send_socket(sock_id,outfile);
}

string dline;
tcpH datH;
void transLayer_sendData(int* syncSeq,int sock_id, char* outfile)//datSeq=syncSeq+1;no ack
{
    //string line;
    int i;
    printf("entered transLayer_sendData\n");

    //preparing datH header
    int ftpPort[16]={0,0,0,0, 0,0,0,0, 0,0,0,1, 0,1,1,0};
    for(i=0;i<16;i++)
    {
        datH.sourcePort[i]=ftpPort[i];
        datH.destinationPort[i]=ftpPort[i];
        datH.urgentPointer[i]=0;
    }
    int* b=BaddOne(syncSeq,32);
    for(i=0;i<32;i++)
    {
        datH.sequenceNumber[i]=b[i];
        datH.ackNumber[i]=b[i];
        datH.windowSize[i]=0;
    }
    //datH.sequenceNumber[23]=1;//as per slide;supposed to be random
    int hLen[4]={0,1,0,1};
    for(i=0;i<4;i++)
    {
        datH.headerLenght[i]=hLen[i];
    }
    for(i=0;i<6;i++)
    {
        datH.reserved[i]=0;
    }
    datH.urg=0;
    datH.ack=0;
    datH.psh=0;
    datH.rst=0;
    datH.syn=0;
    datH.fin=0;
    datH.windowSize[4]=1;//all other were zero's; 2048

    //read data from AO.txt and store it in indata[]
    int dc=0,k=0;

    ifstream ds("AO.txt");
    if(ds.is_open())
    {
        while (ds.good())
        {
            char c=ds.get();
            if(ds.good() && c!=10)
            {
                dline+=c;
                dc++;
            }
        }
        for(k=0;k<dc;k++)
        {
            datH.data[k]=Bctoi(dline[k]);
        }
    }
    else
    {
        cout << "tran_Layer_send datHa: Unable to open file";
    }
    //checksum remaining
    datH.n=dc;
    attach_tcpChecksum(&datH);
    //for(i=0;i<16;i++)
        //datH.checksum[i]=0;
    //tcpWrite(outfile,datH,dc);
    //send_socket(sock_id,outfile);
    ofstream traOutputFile(outfile);
    int data;
    //writing
    if (traOutputFile.is_open())
    {
        for(i=0;i<16;i++)
        {
            traOutputFile<< datH.sourcePort[i];
        }
        //traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< datH.destinationPort[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<32;i++)
        {
            traOutputFile<< datH.sequenceNumber[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<32;i++)
        {
            traOutputFile<< datH.ackNumber[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<4;i++)
        {
            traOutputFile<< datH.headerLenght[i];
        }
        for(i=0;i<6;i++)
        {
            traOutputFile<< datH.reserved[i];
        }
        //flags
        traOutputFile<<datH.urg;
        traOutputFile<<datH.ack;
        traOutputFile<<datH.psh;
        traOutputFile<<datH.rst;
        traOutputFile<<datH.syn;
        traOutputFile<<datH.fin;
        for(i=0;i<16;i++)
        {
            traOutputFile<< datH.windowSize[i];
        }
        traOutputFile<<"\n";
        for(i=0;i<16;i++)
        {
            traOutputFile<< datH.checksum[i];
        }
        for(i=0;i<16;i++)
        {
            traOutputFile<< datH.urgentPointer[i];
        }
        traOutputFile<<"\n";
        data=datH.n;
        //r=data%32;
        if(data != 0)
        {
            for(i=0;i<data;i++)
            {
                traOutputFile<< datH.data[i];
            }
            traOutputFile<<"\n";
        }
        traOutputFile.flush();
        traOutputFile.close();
    }
    else cout << "Unable to open file:tcp_write_in tcp_sendData\n";
}

void transLayer_sendFin(int* seq,int sock_id, char* outfile)
{
    string line;
    int i;
    printf("entered transLayer_sendFin\n");
    tcpH fin;
    //preparing fin header
    int ftpPort[16]={0,0,0,0, 0,0,0,0, 0,0,0,1, 0,1,1,0};
    for(i=0;i<16;i++)
    {
        fin.sourcePort[i]=ftpPort[i];
        fin.destinationPort[i]=ftpPort[i];
        fin.urgentPointer[i]=0;
    }
    //int* b=BaddOne(prevSeq,32);
    for(i=0;i<32;i++)
    {
        fin.sequenceNumber[i]=seq[i];
        fin.ackNumber[i]=0;
        fin.windowSize[i]=0;
    }
    int hLen[4]={0,1,0,1};
    for(i=0;i<4;i++)
    {
        fin.headerLenght[i]=hLen[i];
    }

    for(i=0;i<6;i++)
    {
        fin.reserved[i]=0;
    }
    fin.urg=0;
    fin.ack=0;
    fin.psh=0;
    fin.rst=0;
    fin.syn=0;
    fin.fin=1;
    fin.windowSize[4]=1;//all other were zero's; 2048
    //checksum remaining
    attach_tcpChecksum(&fin);
    tcpWrite(outfile,fin,0);
    //send_socket(sock_id,outfile);
}

void detachTcpH(tcpH dataHeader, char* FtpH_file)
{
    int i;
    ofstream recivedFtpH_file(FtpH_file);
    if (recivedFtpH_file.is_open())
    {
        for(i=0;i<dataHeader.n;i++)
        {
            if(i==8)
                recivedFtpH_file <<"\n";
            if(i==24)
                recivedFtpH_file <<"\n";
            recivedFtpH_file <<dataHeader.data[i];
        }
        recivedFtpH_file.close();
    }
    else
        cout << "Unable to open file:"<<FtpH_file<<"\n";
}
////////////////////////////////////////////////////////////////////////////
int bxor(int l,int m)
{
    int i;
	int a,b,z,x=l-'0',y=m-'0';
	a= x & y;
	b= ~x & ~y;
	z= ~a & ~b;
	return z+'0';
}

char* getFile(IPH ip)
{
    int i;
	ofstream out("temp.txt");
	for(i=0;i<4;i++)
		out<<ip.version[i];
	for(i=0;i<4;i++)
		out<<ip.hlen[i];
	for(i=0;i<8;i++)
		out<<ip.tos[i];
	out<<"\n";
	for(i=0;i<16;i++)
		out<<ip.length[i];
	out<<"\n";
	for(i=0;i<16;i++)
		out<<ip.ident[i];
	out<<"\n";
	for(i=0;i<16;i++)
		out<<ip.flag[i];
	out<<"\n";
	for(i=0;i<8;i++)
		out<<ip.ttl[i];
	for(i=0;i<8;i++)
		out<<ip.protocol[i];
	out<<"\n";
	for(i=0;i<16;i++)
		out<<ip.checksum[i];
	out<<"\n";
	for(i=0;i<32;i++)
	{
		out<<ip.srcadd[i];
		if(i==15)
			out<<"\n";
	}
	out<<"\n";
	for(i=0;i<32;i++)
	{
		out<<ip.destadd[i];
		if(i==15)
			out<<"\n";
	}
	out<<"\n";
	for(i=0;i<ip.n;i++)
	{
		out<<ip.data[i];
		if(i%15==0)
			out<<"\n";
	}
    out.close();
	return (char *)"temp.txt";
}

void getCheckSum(char* chk)
{
    int i;
	ifstream in(chk);
	string temp1,temp2,chksum;
	if(in.is_open())
	{

		getline(in,chksum);
		getline(in,temp2);
		while(in.good())
		{
			for(i=0;i<16;i++)
				chksum[i]=bxor(chksum[i],temp2[i]);
			getline(in,temp2);

			int l=temp2.length();
			if(l<16)
			{
				for(int j=l;j<16;j++)
					temp2+="0";
			}
			//cout<<"temp2 : "<<temp2<<"\n";
			//cout<<chksum<<"\n";
		}
		in.close();
		for(i=0;i<16;i++)
				chksum[i]=bxor(chksum[i],temp2[i]);
		//checksum=new int[16];
		for(i=0;i<16;i++)
		{
			checksum[i]=(chksum[i]-'0');
			//cout<<checksum[i];
		}
		cout<<"\n";
	}
	else cout << "Unable to Open file : getchecsum" << "\n";
	remove(chk);
    //return checksum;
}

IPH netLayer_Read(char *mac,char *toapp, int data)
{
    int i;
    IPH ip;
    string line,dataline;
    ifstream in(mac);
    if(in.is_open())
    {
		/*while(in.good())
		{
			char c=in.get();
			if(in.good() && c!=10)
				line+=c;
		}
		int j=0,t=line.length()-160;
		cout<<"t: "<<t<<"\n";*/
		getline(in,line);
		int j=0;
		for(i=0;i<4;i++)
			ip.version[i]=line[j++]-'0';
		for(i=0;i<4;i++)
			ip.hlen[i]=line[j++]-'0';
		for(i=0;i<8;i++)
			ip.tos[i]=line[j++]-'0';
		for(i=0;i<16;i++)
			ip.length[i]=line[j++]-'0';
        getline(in,line);j=0;
		for(i=0;i<16;i++)
			ip.ident[i]=line[j++]-'0';
		for(i=0;i<16;i++)
			ip.flag[i]=line[j++]-'0';
        getline(in,line);j=0;
		for(i=0;i<8;i++)
			ip.ttl[i]=line[j++]-'0';
		for(i=0;i<8;i++)
			ip.protocol[i]=line[j++]-'0';
		for(i=0;i<16;i++)
			ip.checksum[i]=line[j++]-'0';
        getline(in,line);j=0;
		for(i=0;i<32;i++)
			ip.srcadd[i]=line[j++]-'0';
        getline(in,line);j=0;
		for(i=0;i<32;i++)
			ip.destadd[i]=line[j++]-'0';
        //reading data
        if(data==0)
        {
            for(i=0;i<5;i++)
            {
                getline(in,line);
                dataline+=line;
            }
        }
        else
        {
            for(i=0;i<6;i++)
            {
                getline(in,line);
                dataline+=line;
            }
        }
        j=dataline.length();
        ip.n=j;
		for(i=0;i<j;i++)
			ip.data[i]=dataline[i]-'0';
		in.close();

		//int *chk=getCheckSum(getFile(ip));
		getCheckSum(getFile(ip));
		int *chk=checksum;
		int exec=0;
		for(i=0;i<16;i++)
		{
			//cout<<chk[i];
			if(chk[i]!=0)
				exec++;
		}
		if(exec)
			cout << "\nError in checksum. Terminated " << "\n";
		else
			cout << "\nChecksum Verfied " << "\n";

		netLayer_detach(ip,toapp);
	}
	else cout << "Unable to open file :net_read"<< "\n";

    return ip;
}

void netLayer_write(char* str,IPH iph)
{
    int i;
	ofstream out(str);
	if(out.is_open())
	{

		for(i=0;i<4;i++)
			out<<iph.version[i];
		for(i=0;i<4;i++)
			out<<iph.hlen[i];
		for(i=0;i<8;i++)
			out<<iph.tos[i];
		for(i=0;i<16;i++)
			out<<iph.length[i];
		out<<"\n";
		for(i=0;i<16;i++)
			out<<iph.ident[i];
		for(i=0;i<16;i++)
			out<<iph.flag[i];
		out<<"\n";
		for(i=0;i<8;i++)
			out<<iph.ttl[i];
		for(i=0;i<8;i++)
			out<<iph.protocol[i];
		for(i=0;i<16;i++)
			out<<iph.checksum[i];
		out<<"\n";
		for(i=0;i<32;i++)
			out<<iph.srcadd[i];
		out<<"\n";
		for(i=0;i<32;i++)
			out<<iph.destadd[i];
		out<<"\n";
		for(i=0;i<iph.n;i++)
		{
			if(i%32==0 && i!=0 && i<=160)
				out<<"\n";
			out<<iph.data[i];
		}
		out.close();
	}
	else cout << "Unable to Open File: net_write" << "\n";
}

void netLayer_addIPH(char *netL,char *toMac)
{
    int i;
    IPH iph={0,{0,1,0,0},{0,1,0,1},{0,0,0,1,1,1,1,0},{0},{0},{0},{0,0,0,0,0,1,0,1},{0,0,0,0,0,1,1,0},{0},{0},{0},{0}};
    ifstream in(netL);
	i=0;
	if(in.is_open())
	{

		while(in.good())
		{
			char c=in.get();
			if(in.good() && c!=10)
			{
				iph.data[i++]=c-'0';
				iph.n++;
			}
		}
		in.close();

		int t=(iph.n)/8 + 20;
		for(int j=16;j>=0;j--)
        {
            if(t>0)
            {
                iph.length[j]=t%2;
                t=t/2;
            }
        }

		//int *ckk=getCheckSum(getFile(iph));
		getCheckSum(getFile(iph));
		int *ckk=checksum;
		cout<<"Checksum : ";
		for(i=0;i<16;i++)
		{
			cout<<ckk[i];
			iph.checksum[i]=ckk[i];
		}
		cout<<"\n";

        netLayer_write((char *)toMac,iph);
	}
	else cout << "Unable to Open File : " << netL << "\n";
}

void netLayer_detach(IPH ip,char *toapp)
{
    int i;
    ofstream out(toapp);
    if(out.is_open())
    {
		for(i=0;i<ip.n;i++)
		{
			if(i%32==0 && i!=0 && i<=160)
				out<<"\n";
			out<<ip.data[i];
		}
		out.close();
	}
	else cout << "Unable to Open File:net_detach\n" << "\n";
}

////////////////////////////////////////////////////////////////////////////

int exor(int x, int y)
{
    if(x==y)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int findFirstIndex(int a[],int n)
{
    int i;
    for(i=0;i<n;i++)
    {
        if(a[i]==1)
        {
            return i;
        }
    }
    return -1;
}

frame mac_read(char* macInFileName)
{
    frame out;
    int i,k;
    string line,dataline;
    ifstream macInputFile(macInFileName);
    if (macInputFile.is_open())
    {
        getline (macInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<64;i++)
        {
            out.preamble[i]=Bctoi(line[i]);
        }
        getline (macInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<8;i++)
        {
            out.sfd[i]=Bctoi(line[i]);
        }
        getline (macInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<48;i++)
        {
            out.sourceMac[i]=Bctoi(line[i]);
        }
        getline (macInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<48;i++)
        {
            out.destinationMac[i]=Bctoi(line[i]);
        }
        getline (macInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<16;i++)
        {
            out.type[i]=Bctoi(line[i]);
        }
        getline (macInputFile,dataline);k=0;//cout<<dataline<<"\n";
        k=dataline.length();
        out.n=k;
        if(k != 0)
        {
            for(i=0;i<k;i++)
            {
                out.payload[i]=Bctoi(dataline[i]);
            }
        }
        getline (macInputFile,line);k=0;//cout<<line<<"\n";
        for(i=0;i<32;i++)
        {
            out.crc[i]=Bctoi(line[i]);
        }
        macInputFile.close();
    }
    else cout << "Unable to open file:mac_Read\n";
    cout<<"Frame as read in mac_read:\n";
    printFrame(out);
    checkCrcError(out,CRC32,CRC32_len);
    return out;
}

void mac_write(frame f, char* macOutFileName)
{
    ofstream macOut(macOutFileName);
    int i;
    if(macOut.is_open())
    {
        for(i=0;i<64;i++)
        {
            macOut<<f.preamble[i];
        }
        macOut<<"\n";
        for(i=0;i<8;i++)
        {
            macOut<<f.sfd[i];
        }
        macOut<<"\n";
        for(i=0;i<48;i++)
        {
            macOut<<f.sourceMac[i];
        }
        macOut<<"\n";
        for(i=0;i<48;i++)
        {
            macOut<<f.destinationMac[i];
        }
        macOut<<"\n";
        for(i=0;i<16;i++)
        {
            macOut<<f.type[i];
        }
        macOut<<"\n";
        for(i=0;i<f.n;i++)
        {
            //if(i%32==0 && i!=0) macOut<<"\n";
            macOut<<f.payload[i];
        }
        macOut<<"\n";
        for(i=0;i<32;i++)
        {
            macOut<<f.crc[i];
        }
        macOut<<"\n";
        macOut.close();
    }
    else cout<< "Unable to open: "<<macOutFileName<<"\n";
    //send_socket(macOutFileName);
}

int checkCrcError(frame rf, int* crcKey, int crcKeyLen)//crckeyLen =33 for CRC32
{
    int n=crcKeyLen-1+rf.n;
    int t[n];
    int i,j,k;
    int hex[4];
    j=0;
    for(i=0;i<rf.n;i++)
    {
        t[j++]=rf.payload[i];
    }
    for(i=0;i<32;i++)
    {
        t[j++]=rf.crc[i];
    }
    //printing
    printf("\nFrame recieved: Payload+crc(t[]):-\n");
    hexDisplay(t,n);
    printf("\n");
    //calculating crc
    j=findFirstIndex(t,n);
    while( (n-j) >= crcKeyLen && ( j != -1) )
    {
        for(i=j,k=0; k < crcKeyLen ; i++,k++)
        {
            t[i]=exor(t[i],crcKey[k]);
        }
        j=findFirstIndex(t,n);
    }

    //printing new calc check crc
    printf("\nCHECK CRC of Recieved frame\n");
    hexDisplay(t,n);
    printf("\n");
    //new crc in r
    for(i=rf.n;i<n;i++)
    {
        if(t[i]!=0)
        {
            printf("Frame Reception FAILED; CRC ERROR\n");
            return 0;
        }
    }
    printf("Frame Reception SUCCESSFUL\n");
    return 1;
}

void fixCRC(int* msg,int msgLen,int* crcKey,int crcKeyLen,frame* cf)
{
    int n=msgLen+crcKeyLen-1;
    int m=crcKeyLen-1;
    int t[n];
    int i,j,k,t2,fb,t1;

    //appending zeroes to msg[m(x)] to get t(x);
    for(i=0;i<n;i++)
    {
        if(i<msgLen)
            t[i]=msg[i];
        else
            t[i]=0;
    }
    //dividing
    j=findFirstIndex(t,n);
    while( (n-j) >= crcKeyLen && ( j != -1) )
    {
        for(i=j,k=0; k < crcKeyLen ; i++,k++)
        {
            t[i]=exor(t[i],crcKey[k]);
        }
        j=findFirstIndex(t,n);
    }

    printf("\ncopying crc to frame: printing crc\n");
    for(i=msgLen,k=0 ; i<n ;i++,k++)
    {
        (*cf).crc[k]=t[i];
        printf("%d",t[i]);
    }
    printf("\n");
}

void fixPreamble(frame* cf)
{
    int i;
    for(i=0;i<64;i=i+2)
    {
        (*cf).preamble[i]=1;
    }
    for(i=1;i<64;i=i+2)
    {
        (*cf).preamble[i]=0;
    }
}

void fixSFD(frame* cf)
{
    int i;
    int sfd[8]={1,0,1,0,1,0,1,1};
    for(i=0;i<8;i++)
    {
        (*cf).sfd[i]=sfd[i];
    }
}

void fixType(int* type,frame* cf)
{
    int i;
    for(i=0;i<16;i++)
    {
        (*cf).type[i]=type[i];
    }
}

void fixSrcMac(int* srcMac,frame* cf)
{
    int i;
    for(i=0;i<48;i++)
    {
        (*cf).sourceMac[i]=srcMac[i];
    }
}

void fixDestMac(int* destMac,frame* cf)
{
    int i;
    for(i=0;i<48;i++)
    {
        (*cf).destinationMac[i]=destMac[i];
    }
}

void mac_buildFrame(char* msgFile,char* outFileName)
{
    ifstream macInputFile(msgFile);
    string line;
    frame cf;
    int n,i;
    if (macInputFile.is_open())
    {
        while(macInputFile.good())
        {
            char c=macInputFile.get();
            if(macInputFile.good() && c!=10)
            {
                line+=c;
            }
        }
        n=line.length();
        macInputFile.close();
    }
    else cout<< "Unable to open: "<<msgFile<<"\n";
    cf.n=n;cout<<"N= "<<n<<"\n";
    //fixing payload
    for(i=0;i<n;i++)
    {
        cf.payload[i]=Bctoi(line[i]);
    }
    int src[48]={0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0};
    int dest[48]={0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,1,1,0,1,1,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0};
    int type[16]={0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0};
    fixPreamble(&cf);
    fixSFD(&cf);
    fixSrcMac(src,&cf);
    fixDestMac(dest,&cf);
    fixType(type,&cf);
    fixCRC(cf.payload,n,CRC32,CRC32_len,&cf);
    mac_write(cf,outFileName);
}

void detachFrame(frame cf, char* IpH_file)
{
    int i;
    ofstream macOutputFile(IpH_file);
    if (macOutputFile.is_open())
    {
        for(i=0;i<cf.n;i++)
        {
            if(i%32==0 && i!=0 && i<=320)
                macOutputFile <<"\n";
            macOutputFile <<cf.payload[i];
        }
        macOutputFile.close();
    }
    else
        cout << "Unable to open file:"<<IpH_file<<"\n";
}

void printFrame(frame f)
{
    printf("PAYLOAD SIZE:%d\n",f.n);
    int i,j=0;
    int hex[4];
    int n=f.n;
    n=n*8;
    for(i=0;i<64;i++)
    {
        //printf("%d",f.preamble[i]);
        hex[j++]=f.preamble[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
    printf("|");
    for(i=0;i<8;i++)
    {
        //printf("%d",f.sfd[i]);
        hex[j++]=f.sfd[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
    printf("|");
    for(i=0;i<48;i++)
    {
        //printf("%d",f.destinationMac[i]);
        hex[j++]=f.destinationMac[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
    printf("|");
    for(i=0;i<48;i++)
    {
        //printf("%d",f.sourceMac[i]);
        hex[j++]=f.sourceMac[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
    printf("|");
    for(i=0;i<16;i++)
    {
        //printf("%d",f.type[i]);
        hex[j++]=f.type[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
    printf("|");
    for(i=0;i<n;i++)
    {
        //printf("%d",f.payload[i]);
        hex[j++]=f.payload[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
    printf("|");
    for(i=0;i<32;i++)
    {
        //printf("%d",f.crc[i]);
        hex[j++]=f.crc[i];
        if((i+1)%4==0)
        {
            hex_routine(hex);
            j=0;
        }
    }
    //printf("|");
}

void send_socket(int sock_id, char* socketOut_file)
{
    string line;
    int n;
    ifstream in (socketOut_file);
    if (in.is_open())
   {
       while (in.good())
       {
           char c=in.get();
           if(in.good())
               line+=c;
       }
       n=line.length();
       char msg[n];
       strcpy(msg, line.c_str());
       cout<<"writing this to socket:\n"<<msg<<"\n";
       write(sock_id,msg,sizeof(msg));
       in.close();
   }
   else cout << "Unable to open file: at send socket"<<socketOut_file<<"\n";
}

void recieve_socket(int sock_id, char* socketIn_file, int datalines)
{
    int i;
    int newlines;
    int len;
    if(datalines==0)
    {
        newlines=10;
        len=184+320+32+newlines-1;//for no data
    }
    else
    {
        newlines=11;
        len=184+320+32+12000+newlines;//for max data
    }
    read(sock_id,readBuffer,len);
    cout<<"read DATA as string: \n"<< readBuffer<< "\n";
    ofstream sockWriter(socketIn_file);
    if (sockWriter.is_open())
    {
        for(i=0;i<strlen(readBuffer);i++)
        {
            sockWriter<<readBuffer[i];
        }
        sockWriter.close();
    }
	else
        cout << "Unable to open file:at recieve socket:"<<socketIn_file<<"\n";
}
