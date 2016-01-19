//
// mnc2.c
// CSE 589 FALL
// Programming assignment 2
// Implementation of Distance vector routing protocol: Count to Infinity
// Starts off on a different IP and port depending on its local topology file.
//  Created by swati krishnan on 11/20/15.
//
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<limits.h>

struct ip_table {  //stores all adresses read from file
    int id;
    char ip[255];
    char port[255];
}adr[5];

struct cost_table { //stores initial neighbor costs from file
    int myid;
    int nid;
    int cost;
}costs[4];

struct rt_table {  // Strucure of routing table
    int to_id;
    int next_hop;
    int min_cost;
}rt[5];

struct min_cal{
    int val;
    int nid;
    
}min_in[5];

struct crash {
    int rec[10];
    int rec_len;
}in_int[100];

struct cell {  //main cost matrix for calculating least costs.
    int val;
    int nh;
}cells[5][5];

int cal[5][5];
int dv[5]; //The distance vector that is sent
int dv_new[5];
int dv_up[5];
int servers;
int nb;
int packets;
int count;
int crash_ch[50];
int neb[4]; //maintaining neighbors array to find out crash
int hid;//host id


#define STDIN 0
#define MAX_ARGS 10
#define MAX_LEN  100
#define inf 65535

void send_to_nb(int i,int s,int f);//send to all neighbors
void min_dist(int i, int j);/// find out minimum from all neighbors
int compare(int dv[5], int dv_new[5]);
void show();
void send_inf(int i,int f); // function to send message on infinity update
void send_link(int i,int up_cost,int f); // function to send message on other than infinity update

int main(int argc, char **argv) {
    
    
    FILE *fp;
    char buff[255];
    //int hid;
    packets=0;
    count=0;
    int cur_neb=0; //for within neb
    int int_no=1;
    int r=0; //for received packets in interval
    
    //----------open file and get no of servers and neighbours
    fp = fopen(argv[2], "r");
    fgets(buff, 255, (FILE*)fp);
    //printf("no of servers: %s\n", buff );
    servers = atoi(buff);
    //printf("no of servers %d\n", servers );
    
    //printf("no of neighbours in int: %d\n",nb);
    fgets(buff, 255, (FILE*)fp);
    //printf("no of neighbours %s\n", buff );
    nb = atoi(buff);
    //printf("no of neighbours in int: %d\n",nb);
    //fclose(fp);
    
    //---------ge all server ips and ports in ADR
    for(int i=0;i<servers;i++) {
        
        fscanf(fp, "%s", buff);
        adr[i].id=atoi(buff);
        fscanf(fp, "%s", adr[i].ip);
        //adr[0].ip=buff;
        fgets(adr[i].port, 255, (FILE*)fp);
        //adr[i].port=buff;
        //printf("in table id ip and port are %d %s %s \n",adr[i].id,adr[i].ip,adr[i].port);
    }
    
    
    //fclose(fp);
    
    //-------GET ALL MYID, NID AND COST IN COSTS ARRAY COSTS----------
    for(int i=0;i<nb;i++) {
        
        
        fscanf(fp, "%s", buff);
        costs[i].myid=atoi(buff);
        fscanf(fp, "%s", buff);
        costs[i].nid=atoi(buff);
        fgets(buff, 255, (FILE*)fp);
        costs[i].cost=atoi(buff);
        //printf("in table myid nid and cost are %d %d %d \n",costs[i].myid,costs[i].nid,costs[i].cost);
    }
    fclose(fp);
    //printf("my server id is: %d\n",costs[0].myid);
    int f;
    for(f=0;f<5;f++) {
        if(adr[f].id==costs[0].myid)
            break;
    }
    
    //-----------IP AND PORT OF HOST-----------------------------
    
    printf("id ip and port of host server found in iptable is %d %s %s \n",adr[f].id,adr[f].ip,adr[f].port);
    hid=adr[f].id;
    int up_int=atoi(argv[4]);
    printf("update interval is %d\n",up_int);
    //printf("UINT_MAX = %u \n", inf);
    
    //-------INITIAL DISTANCE VECTOR--------------
    
    //int dv[servers];
    for(int i=0;i<servers;i++) {
        dv[i]=-1;
        //printf("\t %d",dv[i]);
    }
    //printf("\n");
    
    for(int j=0;j<nb;j++) {
        dv[costs[j].nid-1]=costs[j].cost;
    }
    
    for(int i=0;i<servers;i++) {
        if(i==(costs[0].myid-1))
            dv[i]=0;
        if(dv[i]==-1)
            dv[i]=inf;
    }
    //printf("FINAL DV IS\n");
    
    /*for(int i=0;i<servers;i++) {
     printf("\t %d",dv[i]);
     }*/
    //--------INITIAL ROUTING TABLE--------
    
    for(int i=0;i<servers;i++) {
        rt[i].to_id=i+1;
        if(dv[i]==0) {
            rt[i].next_hop=rt[i].to_id;
            rt[i].min_cost=0;
        }
        if(dv[i]==inf) {
            rt[i].min_cost=inf;
            rt[i].next_hop=-1;
        }
        else {
            rt[i].next_hop=rt[i].to_id;
            rt[i].min_cost=dv[i];
        }
    }
    //----------INITIAL CALC CELLS TABLE--------
    
    /*struct cell {
     int val;
     int nh;
     }cal[5][5];*/
    //printf("INIT CAL TABLE\n");
    for(int i=0;i<servers;i++) {
        for(int j=0;j<servers;j++) {
            if(i==j) {
                cells[i][j].val=0;
                cells[i][j].nh=i+1;
            }
            else {
                cells[i][j].val=inf;
                cells[i][j].nh=-1;
            }
            //printf("%d,%d\t",cells[i][j].val,cells[i][j].nh);
        }
        //printf("\n");
        
    }
    for(int i=0;i<servers;i++) {
        if(i==adr[f].id-1) {
            for(int j=0;j<servers;j++) {
                cells[i][j].val=dv[j];
                if(cells[i][j].val==inf)
                    cells[i][j].nh=-1;
                else
                    cells[i][j].nh=j+1;
            }
        }
    }
    //---------------------- CREATING UDP SOCKET----------------------------
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    //char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    int sockfd;
    //hints.ai_flags = AI_PASSIVE; // use my IP
    //printf("AGAIN id ip and port of host server found in iptable is%s- \n",adr[f].port);
    //printf("con to int -%d-",atoi(adr[f].port));
    char str[20];
    sprintf(str, "%d",atoi(adr[f].port));
    //printf("STRING FORM -%s-",str);
    if ((rv = getaddrinfo(adr[f].ip,str, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    
    //printf("SOCKET MADE AT %d\n",sockfd);
    freeaddrinfo(servinfo);
    //printf("listener: waiting to recvfrom...\n");
    
    int numbytes;
    struct sockaddr_in their_addr;
    char buf[100];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    addr_len = sizeof their_addr;
    //for tokenising
    char* token;
    int dvin[5]; //incoming dv
    char tokbuf[20];
    
    int up_from;
    int up_to;
    int up_cost;
    int rem[4];
    int in_rem=0;
    
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    struct timeval tv;
    tv.tv_sec = atoi(argv[4]); //set numbe rof seconds
    tv.tv_usec = 0;
    
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    
    FD_SET(STDIN, &master);
    FD_SET(sockfd, &master);
    
    char input[MAX_LEN],*cp;
    const char *delim = " \t\n"; //delimiters of commands
    
    // keep track of the biggest file descriptor
    fdmax = sockfd;
    
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
            perror("select");
            exit(4);
        }
        //printf(">");
        if (FD_ISSET(STDIN, &read_fds))   //first checking standard input
        {//printf("A key was pressed!\n");
            if (fgets(input, sizeof(input), stdin) == NULL) {
                /* inputed EOF(Ctrl-D) */
                printf("Goodbye!\n");
                exit(0);
            }
            
            //split the input to commands with blank and Tab
            cp = input;
            for (argc = 0; argc < MAX_ARGS; argc++) {
                if ((argv[argc] = strtok(cp,delim)) == NULL)
                    break;
                cp = NULL;
            }
            if(strcmp(argv[0], "creator") == 0) {
                printf("name:Swati Krishnan   ubit:swatikri   ubMail:swatikri@buffalo.edu\n");
                //exit(0);
            }
            if(strcmp(argv[0], "display") == 0) {
                printf("%s SUCCESS.\n",argv[0]);
                printf("ROUTING TABLE\n");
                printf("to_id\tnext_hop\tmin_cost\n");
                
                for(int i=0;i<servers;i++) {
                    printf("%d\t%d\t\t%d\n",rt[i].to_id,rt[i].next_hop,rt[i].min_cost);
                }
                //exit(0);
            }//end of display
            if(strcmp(argv[0], "step") == 0) {
                printf("%s SUCCESS.\n",argv[0]);
                for(int i=0;i<nb;i++)
                    send_to_nb(i,servers,f);
            }
            if(strcmp(argv[0], "packets") == 0) {
                printf("%s SUCCESS\n",argv[0]);
                printf("%d PACKETS RECEIVED.\n",packets);
                packets=0;
            }
            if(strcmp(argv[0], "update")==0) {
                //printf("from %d\n",atoi(argv[1]));
                up_from=atoi(argv[1]);
                up_to=atoi(argv[2]);
                if(strcmp(argv[3], "inf") == 0)
                    up_cost=inf;
                else up_cost=atoi(argv[3]);
                //printf("COST IS %d\n",up_cost);
                int check;
                //int did=atoi(argv[1]);
                //printf("DIS %d\n",did);
                for(check=0;check<nb;check++) {    //check for neighbors
                    if(costs[check].nid==up_to)
                        break;
                }
                if(check!=nb) {
                    printf("%s SUCCESS\n",argv[0]);          //if neighbor found
                    if(up_cost==inf) {
                        send_inf(up_to,f);//send infinity change msg to neighbor
                        cells[up_from-1][up_to-1].val=inf;
                        cells[up_to-1][up_from-1].val=inf;
                        
                        for(int i=0;i<servers;i++) {       //then find new min values
                            if(i==up_from-1) {
                                for(int j=0;j<servers;j++) {
                                    if(i!=j) {
                                        //printf("BEFORE CALLIN MIN_DIST for %d %d\n",i,j);
                                        min_dist(i,j);
                                    }
                                }
                            }
                        } //end of using new dv to find new min costs
                        
                    }
                    else {
                        send_link(up_to,up_cost,f);
                        cells[up_from-1][up_to-1].val=up_cost;
                        cells[up_to-1][up_from-1].val=up_cost;
                        for(int i=0;i<servers;i++) {
                            if(i==up_from-1) {
                                for(int j=0;j<servers;j++) {
                                    if(i!=j) {
                                        //printf("BEFORE CALLIN MIN_DIST for %d %d\n",i,j);
                                        min_dist(i,j);
                                    }
                                }
                            }
                        } //end of using new dv to find new min costs
                        
                    }
                }
                
                else
                    printf("UPDATE:NOT A NEIGHBOR\n");
                
            }//end of update
            
            if(strcmp(argv[0], "disable")==0) {
                int check;
                int did=atoi(argv[1]);
                //printf("DIS %d\n",did);
                for(check=0;check<nb;check++) {
                    if(costs[check].nid==did)
                        break;
                }
                if(check==nb)
                    printf("DISABLE:NOT A NEIGHBOR\n");
                else {
                    //printf("%s SUCCESS\n",argv[0]);
                    for(int j=0;j<servers;j++) {
                        if(cells[hid-1][j].nh==did && cells[hid-1][j].nh!=hid) {
                            cells[hid-1][j].val=inf;
                            cells[j][hid-1].val=inf;
                        }
                    }
                    
                    
                    for(int i=0;i<servers;i++) {
                        if(rt[i].next_hop==did) {
                            rt[i].next_hop=-1;
                            rt[i].min_cost=inf;
                        }
                    }
                    
                    for(int i=0;i<servers;i++) {
                        if(i==up_from-1) {
                            for(int j=0;j<servers;j++) {
                                if(i!=j) {
                                    //printf("BEFORE CALLIN MIN_DIST for %d %d\n",i,j);
                                    min_dist(i,j);
                                }
                            }
                        }
                    }
                    
                    printf("%s SUCCESS\n",argv[0]);
                    
                }
            }//end of disable
            if(strcmp(argv[0], "crash")==0) {  //exit program on crash and let the neighbors handle it
                exit(0);
            }//end of crash
            
            
        } // end of reading stdin loop
        //checking sockfd for reeceived packets
        else if (FD_ISSET(sockfd, &read_fds)) {
            if ((numbytes = recvfrom(sockfd, buf, 100 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
            }
            packets++;
            
            buf[numbytes] = '\0';
            //printf("listener: packet contains \"%s\"\n", buf);
            //printf("NOW TOKENISING\n");
            if(numbytes==1) {                  //if infinity message then change costs of those servers that use link as next hop. Only has the id of the server that sends.
                int inf_from=atoi(buf);
                printf("RECEIVED A MESSAGE FROM SERVER %d\n",inf_from);
                //printf("LINK FROM HID %d to %d\n",hid,inf_from);
                cells[hid-1][inf_from-1].val=inf;
                cells[inf_from-1][hid-1].val=inf;
                //find new min costs
                for(int i=0;i<servers;i++) {
                    if(i==hid-1) {
                        for(int j=0;j<servers;j++) {
                            if(i!=j) {
                                min_dist(i,j);
                            }
                        }
                    }
                }
                
            }
            else if(numbytes==3) {  //if link update message with cost. has id from and cost
                char *token1;
                char *token2;
                char tokbuf1[10];
                token1 = strtok(buf, ",");
                token2 = strtok(NULL, buf);
                //printf("LINK FROM %s\n",token1);
                sprintf(tokbuf1,"%s",token1);
                int from_id=atoi(tokbuf1);
                //printf("LINK FROM %s\n",token2);
                sprintf(tokbuf1,"%s",token2);
                int link=atoi(tokbuf1);
                printf("RECEIVED A MESSAGE FROM SERVER %d\n",from_id);
                //printf("FROM %d and LINK %d\n",from_id,link);
                cells[hid-1][from_id-1].val=link;
                cells[from_id-1][hid-1].val=link;
                for(int i=0;i<servers;i++) {
                    if(i==hid-1) {
                        for(int j=0;j<servers;j++) {
                            if(i!=j) {
                                min_dist(i,j);
                            }
                        }
                    }
                }
                
                
            }
            else { //for all other packet received messages
                int t=0;
                token = strtok(buf,",");   //tokenising on comma storing in dvin array
                while( token != NULL )
                {
                    //printf( " %s\n", token );
                    sprintf(tokbuf,"%s",token);
                    dvin[t]=atoi(tokbuf);
                    //printf("IN DV %d\n",dvin[t]);
                    t++;
                    token = strtok(NULL, ",");
                }
                
                printf("RECEIVED A MESSAGE FROM SERVER %d\n",dvin[0]); //storing incoming dv packet's values
                for(int i=0;i<servers;i++) {
                    if(i==dvin[0]-1) {
                        for(int j=0,k=1;j<servers;j++,k++) {
                            cells[i][j].val=dvin[k];
                            cells[j][i].val=dvin[k];
                        }
                    }
                }
                //printf("RECEIVED A MESSAGE FROM SERVER %d\n",dvin[0]);
                int q;//to check in neb array
                if(cur_neb==0) {
                    neb[cur_neb]=dvin[0];      // adding server id if not in current neighbor array
                    cur_neb++;
                }
                else {
                    for(q=0;q<cur_neb;q++)
                        if(neb[q]==dvin[0])
                            break;
                    if(q==cur_neb) {
                        neb[cur_neb]=dvin[0];
                        cur_neb++;
                    }
                    
                } //end of else for putting into neb
                
                /*printf("CUR NB ARRAY\n");
                 for(int i=0;i<cur_neb;i++)
                 printf("%d,",neb[i]);
                 printf("\n");*/
                
                
                in_int[int_no].rec[r]=dvin[0]; //adding the server id(that it received from) in current interval received packets.
                r++;
                
                for(int i=0;i<servers;i++) {
                    if(i==hid-1) {
                        for(int j=0;j<servers;j++) {  //finding minimum cost values
                            if(i!=j) {
                                min_dist(i,j);
                            }
                        }
                    }
                }
                
            }
            
        } //end of reading sock
        
        else {
            /*for(int i=0;i<nb;i++)
             send_to_nb(i,servers,f);*/
            count++;
            
            in_int[int_no].rec_len=r;   //here we find out if any neighbor has crashed or not.
            //printf("NO RECEIVED IN THIS INT %d\n",in_int[int_no].rec_len);
            if(int_no==3) {    //flush in_int after 3 intervals so check for 3, like int_no%3
                //int flag=0;
                //printf("CHECK IN %d interval\n",int_no);
                for(int i=0;i<cur_neb;i++) {
                    int flag=0;
                    for(int j=1;j<=3;j++) {
                        for(int k=0;k<in_int[j].rec_len;k++) {
                            if(in_int[j].rec[k]==neb[i])    // if cur_nb is found at least once in the 3 intervals
                                flag=1;
                            //break;
                            
                        }
                    }
                    if(flag==0) {
                        printf("SERVER %d HAS CRASHED\n",neb[i]); //if not found then add it to remove array
                        rem[in_rem]=neb[i];
                        in_rem++;
                    }
                    
                }//change cur_neb to check
                //printf("NBs TO BE REMOVED\n");
                for(int i=0;i<in_rem;i++) {
                    //printf("%d server has crashed",rem[i]);
                    //dv[rem[i]-1]=inf;
                    
                    for(int j=0;j<servers;j++) {
                        if(cells[hid-1][j].nh==rem[i]) {  //set value to inf for every crashed server
                            cells[hid-1][j].val=inf;
                            cells[j][hid-1].val=inf;
                        }
                    }
                }
                
                for(int i=0;i<in_rem;i++) {
                    for(int j=0;j<cur_neb;j++) {  //change current nb array after removal
                        if(rem[i]==neb[j]) {
                            neb[j]=neb[j+1];
                            cur_neb--;
                        }
                    }
                }
                for(int i=0;i<4;i++) //clear to remove array
                    rem[i]=0;
                in_rem=0;
                int_no=0; // set int_no back to 0 after 3 intervals
            }//close int_no 3
            //int_no=0;
            
            //}//stop checking for int_no 3
            int_no++;
            r=0;
            for(int i=0;i<nb;i++)         //timeout sending pf packets
                send_to_nb(i,servers,f);
        }
    }//end of for ;; loop
    
    
    //close(sockfd);
} //end main

int compare(int dv[5], int dv_new[5]) { //compares new and old dvs. Not used but can be for debugging
    for(int i=0;i<servers;i++) {
        if(dv_new[i]!=dv[i])
            return 1;
    }
    return 0;
}
void show() {   //shows current dv
    for(int i=0;i<servers;i++) {
        printf("\t %d",dv[i]);
    }
}

void min_dist(int i, int j) {
    //int min_in[5]; //see if you must flush values
    //min_in[0]=dv[j];
    for(int i=0;i<5;i++) {
        min_in[i].val=0;
        min_in[i].nid=0;
    }
    int k=0;//k becomes size of min array ofe each neighbour
    int mini;
    int hop_nb;
    for(int l=0;l<nb;l++) {  //Applying Bellman Ford
        min_in[k].val=cells[i][(costs[l].nid)-1].val+cells[(costs[l].nid)-1][j].val;
        min_in[k].nid=costs[l].nid;
        //printf("distance from %d to %d with nb %d in ID is %d\n",i+1,j+1,costs[l].nid,min_in[k].val);
        k++;
    }
    //Finding miminum out of all neighbor values.
    mini=min_in[0].val;
    hop_nb=min_in[0].nid;
    for(int m=0;m<k-1;m++) {
        if(min_in[m+1].val<min_in[m].val) {
            mini=min_in[m+1].val;
            hop_nb=min_in[m+1].nid;
        }
    }
    //printf("MIN VALUE FOR %d IS %d with next hop %d\n",j+1,mini,hop_nb);
    cells[i][j].val=mini;       //updating cost matrix
    cells[i][j].nh=hop_nb;
    dv[j]=mini;                 //updating dv
    //dv_new[i]=0;
    
    rt[j].next_hop=hop_nb;  //updating routing table
    rt[j].min_cost=mini;
    
    
    
}

//---------------------------SEND TO ALL NEIGHBORS--------------------------------


void send_to_nb(int i, int servers,int f) {
    int sockfd1;
    
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    //char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    
    //printf("ip and port of neigh is -%s-  -%s-",adr[costs[i].nid-1].ip,adr[costs[i].nid-1].port);
    //printf("con to int -%d-",atoi(adr[costs[i].nid-1].port));
    char str1[20];
    sprintf(str1, "%d",atoi(adr[costs[i].nid-1].port));
    //printf("STRING FORM -%s-\n",str1);
    
    
    //-----send to neigh---------
    int numbytes;
    if ((rv = getaddrinfo(adr[costs[i].nid-1].ip,str1, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        //return 1;
    }
    
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd1 = socket(p->ai_family, p->ai_socktype,
                              p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        //return 2;
    }
    char strbuff[200];
    char strfin[200];
    char strhost[10];
    //printf("ID OF SERVER %d\n",adr[f].id);
    sprintf(strhost, "%d,",adr[f].id);
    //printf("NOW IN STRING -%s-",strhost);
    strcat(strfin,strhost);
    //printf("socket to neighbour %d",sockfd1);
    for(int i=0;i<servers;i++) {
        sprintf(strbuff,"%d,",dv[i]);
        strcat(strfin,strbuff);
        //printf("strfin ite -%s-\n",strfin);
    }
    //printf("FINAL STRING TO BE SENT -%s-",strfin);
    //strfin[0]='\0';
    
    if ((numbytes = sendto(sockfd1, strfin, strlen(strfin), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    
    freeaddrinfo(servinfo);
    
    //printf("talker: sent %d bytes to %s %s\n", numbytes, adr[costs[0].nid-1].ip, str1);
    close(sockfd1);
    strfin[0]='\0';
}

//---------------------------SEND INFINITY MESSAGE-------------------------

void send_inf(int i, int f) {
    int sockfd1;
    
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    //char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    
    //printf("ip and port of neigh is -%s-  -%s-",adr[costs[i].nid-1].ip,adr[costs[i].nid-1].port);
    //printf("con to int -%d-",atoi(adr[costs[i].nid-1].port));
    char str1[20];
    sprintf(str1, "%d",atoi(adr[i-1].port));
    //printf("STRING FORM -%s-\n",str1);
    
    
    //-----send to neigh---------
    int numbytes;
    if ((rv = getaddrinfo(adr[i-1].ip,str1, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        //return 1;
    }
    
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd1 = socket(p->ai_family, p->ai_socktype,
                              p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        //return 2;
    }
    //char strbuff[200];
    char strfin[] = "inf";
    char strhost[10];
    sprintf(strhost, "%d",adr[f].id);
    
    if ((numbytes = sendto(sockfd1, strhost, strlen(strhost), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    
    freeaddrinfo(servinfo);
    
    //printf("talker: sent INF %d bytes to %s %s\n", numbytes, adr[i-1].ip, str1);
    close(sockfd1);
    strfin[0]='\0';
}

//-------------------------------SEND LINK UPDATE MESSAGE------------------------------

void send_link(int i,int up_cost,int f) {
    int sockfd1;
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    //char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    
    //printf("ip and port of neigh is -%s-  -%s-",adr[costs[i].nid-1].ip,adr[costs[i].nid-1].port);
    //printf("con to int -%d-",atoi(adr[costs[i].nid-1].port));
    char str1[20];
    sprintf(str1, "%d",atoi(adr[i-1].port));
    //printf("STRING FORM -%s-\n",str1);
    
    
    //-----send to neigh---------
    int numbytes;
    if ((rv = getaddrinfo(adr[i-1].ip,str1, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        //return 1;
    }
    
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd1 = socket(p->ai_family, p->ai_socktype,
                              p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        //return 2;
    }
    //char strbuff[200];
    //char strfin[] = "inf";
    char strhost[10];
    char strcost[10];
    sprintf(strhost, "%d,",adr[f].id);
    sprintf(strcost, "%d",up_cost);
    //printf("HOST STRING IS -%s-\n",strhost);
    strcat(strhost,strcost);
    
    //printf("INFINITY STRING TO BE SENT -%s-",strhost);
    //strfin[0]='\0';
    
    if ((numbytes = sendto(sockfd1, strhost, strlen(strhost), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    
    freeaddrinfo(servinfo);
    
    //printf("talker: sent INF %d bytes to %s %s\n", numbytes, adr[i-1].ip, str1);
    close(sockfd1);
    strhost[0]='\0';
    strcost[0]='\0';
}


