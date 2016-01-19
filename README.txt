

-The program needs a file and and update interval as arguments to start.

-It reads those files, finds out the total servers in the network, the host’s neighbors, its costs to them and stores them in the commented structures.

-The maximum value of infinity has been chosen as 65535 in the macro. Updating a link for a higher number or using a topology file with a greater cost may cause problems. Change the macro if you have to use higher values.

-Once all the initial tables and arrays such as the routing table, cost calculate matrix (cells[][]),initial dv are set up the UDP socket is made.
Here just ipv4 is used, but it can be extended to ipv6 by adding cast to struct sockaddr_storage.

-Routing table :

struct rt_table {  // Strucure of routing table
    int to_id;
    int next_hop;
    int min_cost;
}rt[5];

is defined in line 35.

-Distance vector:

int dv[5];
is defined in line 58.

-cost matrix:
struct cell {  //main cost matrix for calculating least costs.
    int val;
    int nh;
}cells[5][5];
is defined in line 52.
used to calculate neighbor values for Bellman-Ford application.

-After creating the UDP socket-sockfd, we add it and STDIN to FD_SET for listening on both.

-the timeout values is specified so that the packets are sent irrespective of whether any of the above events happen or not.

-The update command, for eg: 1 2 inf will first send a link changed to infinity message from server 1 to 2. It will then make the necessary changes to its cost matrix i.e cells. min_distance will then be called on every server 1 to other server in cost matrix pair to find out new minimum distance and THEN routing table will be updated. Thus, routing table will only have infinity values if no other least cost path is found.

-In case of infinity change, all the servers that use the link as next hop will also be changed to infinity.

-Similarly, any other update command, for eg, update 1 2 4 will first send link change message from 1 to 4 (with cost of course), then make changes in its cost matrix and then call min_dist. The routing table will reflect the least costs after the update change.

-The inf message is just one byte(containing server id; we assume that ids are from 0-9. The update message has the server id from and new cost.So 3 bytes including comma for tokenization.

-Disable works similarly. Both commands check for neighbors.

-Crash just exits the program. The neighbors are constantly checking the packets they receive in every interval. If a “current” neighbor i.e a neighbor that has sent at most one packet does not have a packet in consecutive 3 intervals then the neighbor finds out that it has crashed.

struct crash {
    int rec[10];
    int rec_len;
}in_int[]; is used for this. 

Since in_int is flushed after every 3 intervals, the minimum size of in_int[] must be 3.



