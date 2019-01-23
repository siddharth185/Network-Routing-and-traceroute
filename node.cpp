#include<iostream>
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <map>
#include <vector>
#include<mutex>
#include <fstream>
#include<stdio.h>
#include <sstream>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <netdb.h>


using namespace std;
#include "utils.h"

int totalNodes = 0;

std::mutex rtable_mutex;

struct NodeInfo{
	char* ip;
	u_short port;
};

struct data_packet {
	uint8_t type; //type 1 = Routetrace msg. type 2 = New routetracemsg by client
	uint8_t src; // type 1 = msg sent by. type2 = init traceroute from this. 
	uint8_t trig_by; //Who triggered this Routetrace
	uint8_t dest; //type1 = msg sent to. type2 -traceroute destination
	uint8_t packetId; // to remember orig src and dest 
	uint8_t ttl; // current node_id
	uint8_t orig_dest;
	uint8_t orig_src;
	uint8_t rt_ttl; // Routetrace ttl by node starting it..0, 1 ,2 ..
	uint8_t responder_id; // Node who sent back packet 
	
};

struct packet {
	uint8_t sourceId;
	uint8_t destinationId;
	uint8_t packetId;
	uint8_t timeToLive;
};
struct TableEntry{
	int destination;
	int next_hop; //node_id of the next hop
	int distance; //distance in hops
	int sender_nodeID;
};
struct VectorEntry {
	int destination;
	int next_hop;
	int distance;
};

struct Node {
	int id;
	string hostname;
	struct in_addr **addr_list;
	char ip[100];
	int controlPort;
	int dataPort;
	//int* neighborIDs;
	vector<int> neighborIDs;
	map<int, Node> neighbors;
	int numNeighbors;
	packet routetracePacket;
};
Node* nodeList;

struct Message{
	int type;
	int source;
	int destination;
	int next_hop; //node_id of the next hop
	int distance; //distance in hops
};
// 

int update(std::map<int, VectorEntry>& rtable, VectorEntry entry, int node_rec_from)
{
	//int newDistance = rtable.at(entry.destination).distance + 1;
	// std::cout << "Inside Update. Node recv from" << node_rec_from <<endl;
	if (rtable.count(entry.destination) == 0) //destination not found in rtable, case 1
	{	VectorEntry temp = {entry.destination, node_rec_from, entry.distance + 1};
		rtable.insert(std::make_pair(entry.destination, temp));
		std::cout << "New entry added to rtable from " << node_rec_from <<std::endl;


		return 0;
	}

	// if (rtable.find(entry.destination) != rtable.end()) {
		if (entry.next_hop != node_rec_from) {
			if ( rtable.at(entry.destination).distance > entry.distance + 1 ) {
				VectorEntry temp = {entry.destination, node_rec_from, entry.distance+1};
				rtable[entry.destination] = temp;
				std::cout << "Updated rtable with node from " << node_rec_from <<std::endl;
				return 0;
			}
			// else
			// 	{std::cout << "No update" <<std::endl;}
		}
		else {
			VectorEntry temp = {entry.destination, node_rec_from, entry.distance+1};
			rtable[entry.destination] = temp;
			std::cout << "Updated rtable with node from " << node_rec_from <<std::endl;
			return 0;
		}
	// }
	// std::cout << "Vector Table Stable" << std::endl;
	return 0;
}

NodeInfo GetNodeInfo( int totalNodes, int node_search) // get info of given node
{	
	for (int i = 0; i < totalNodes; i++) {
		if (nodeList[i].id == node_search) {
			NodeInfo nodeinfo = {nodeList[i].ip, (u_short)(nodeList[i].dataPort)};
			return nodeinfo;
		}
	}	
}

int data_thread(int s, int my_node_id, std::map<int, Node>& neigbours, std::map<int, VectorEntry>& rtable){

		cout << "Starting data_thread" << endl;
		// std::map<int, int> TTLmap; // Remembers ttl values of packet id of current . key - packet_id, value - TTL
		int curr_packet_id = 0;
		NodeInfo nodeinfo;
		// std::map<int, int> TTLmap; // maps 
		socklen_t from_len = sizeof(struct sockaddr);
		struct data_packet entry_received;
		struct sockaddr_in received_from, dest_info;
		memset(&received_from, 0, sizeof(received_from));
		int to_send;
		while(1)
		{

		
		int rec = recvfrom(s, &entry_received, sizeof(entry_received), 0, (struct sockaddr*) &received_from, &from_len);
		if (entry_received.type == 1) // routetrace msg
		{
			if (entry_received.trig_by == my_node_id && entry_received.dest == my_node_id) // trig by me and returned back to me
			{	cout << "Recived TTL 0 msg" << endl;
				if (entry_received.orig_dest == entry_received.responder_id)
				{
					cout << "Route Trace complete for packet_id " << unsigned(entry_received.packetId) << "Final Hop - " << unsigned(entry_received.responder_id) << std::endl; 
				}
				else
				{ //route trace incomplete
				std::cout << "Intermediate hop for PacketID " <<unsigned(entry_received.packetId) <<" " << unsigned(entry_received.src) << std::endl;
				entry_received.rt_ttl += 1;
				entry_received.ttl = entry_received.rt_ttl;
				entry_received.dest = entry_received.orig_dest;
				entry_received.src = entry_received.orig_src;

				//TODO - Look at Rtable and forward to next hop

				to_send = rtable.at(entry_received.dest).next_hop;
				nodeinfo = GetNodeInfo(totalNodes ,to_send);

				memset(&dest_info, 0, sizeof(dest_info));

				dest_info.sin_family = AF_INET;
				dest_info.sin_port = nodeinfo.port;
				inet_aton(nodeinfo.ip, &dest_info.sin_addr);

				sendto(s, &entry_received, sizeof(entry_received), 0, (struct sockaddr *) &dest_info,  sizeof(dest_info)); 


				}



			}
			else // intermediate node
			{
				if(entry_received.ttl == 0) //send back to node which trig TODO - Modify to travel through hops 
				{
					// entry_received.ttl = 15;
					// int source = entry_received.source 
					// entry_received.source = entry_received.dest
					// entry_received.dest = source
					//send package
					cout << "Intermediate Node for route trace - TTL0.Sending back to " << unsigned(entry_received.orig_src) << std::endl;
					//get node info
					sockaddr_in dest_info;
					memset(&dest_info, 0, sizeof(dest_info));
					entry_received.ttl = 15;
					entry_received.src = my_node_id;
					entry_received.dest = entry_received.trig_by;
					entry_received.responder_id = my_node_id;
					//check rtable and send back

					to_send = rtable.at(entry_received.dest).next_hop;
					nodeinfo = GetNodeInfo(totalNodes ,to_send);

					memset(&dest_info, 0, sizeof(dest_info));

					dest_info.sin_family = AF_INET;
					dest_info.sin_port = nodeinfo.port;
					inet_aton(nodeinfo.ip, &dest_info.sin_addr);
					// cout << "Next hop " << to_send << endl;
					// cout << " ip - " << nodeinfo.ip << endl;
					// cout << " port - " << nodeinfo.port << endl;
					int status = sendto(s, &entry_received, sizeof(entry_received), 0, (struct sockaddr *) &dest_info,  sizeof(dest_info)); 
					if(status < 0)
						perror("socket interm node");
					//Send package

				}
				else // non zero TTL
				{	cout << "Intermediate Node for route trace with non zero TTL. Forwarding " << std::endl;
					entry_received.ttl -=1;

					to_send = rtable.at(entry_received.dest).next_hop;
					nodeinfo = GetNodeInfo(totalNodes ,to_send);

					memset(&dest_info, 0, sizeof(dest_info));

					dest_info.sin_family = AF_INET;
					dest_info.sin_port = nodeinfo.port;
					inet_aton(nodeinfo.ip, &dest_info.sin_addr);
					// cout << "Next hop " << to_send << endl;
					// cout << " ip - " << nodeinfo.ip << endl;
					// cout << " port - " << nodeinfo.port << endl;
					int status = sendto(s, &entry_received, sizeof(entry_received), 0, (struct sockaddr *) &dest_info,  sizeof(dest_info));
					if(status < 0)
						perror("socket");
					//TODO - See Rtable and forward to next hop for this destination
				}
			}
		}

	else if (entry_received.type == 2) // type 2 msg, received from client for new routetrace
	{	
		// packet_id+=1 ;
		// int new_packet_id = packet_id;
		// curr_packet_id+=1;
		cout << "===================Initiating New Route Trace to - " <<unsigned(entry_received.dest)<< " Packet ID - " << unsigned(entry_received.packetId) << endl;

		sockaddr_in dest_info;
		struct data_packet send_packet;
		
// 		//Trigger new route trace to dest, send a type 1 message to src
// 		// 
		// TTLmap[new_packet_id]++; //set packet_id value to 0

		//build data_packet
		send_packet.type = 1;
		send_packet.trig_by = entry_received.src;
		send_packet.src =  entry_received.src;
		send_packet.packetId = entry_received.packetId;
		send_packet.dest = entry_received.dest;
		send_packet.rt_ttl = 0;
		send_packet.ttl = 0;
		send_packet.orig_src = my_node_id;
		send_packet.orig_dest = send_packet.dest;

		to_send = rtable.at(send_packet.dest).next_hop;
		nodeinfo = GetNodeInfo(totalNodes ,to_send);
		// cout << "Next hop " << to_send << endl;
		// cout << " ip - " << nodeinfo.ip << endl;
		cout << nodeinfo.port << endl;
		memset(&dest_info, 0, sizeof(dest_info));

		dest_info.sin_family = AF_INET;
		dest_info.sin_port = nodeinfo.port;
		inet_aton(nodeinfo.ip, &dest_info.sin_addr);
// inet_aton(itr_n->second.ip, &servaddr.sin_addr);
		// some_addr = inet_ntoa(antelope.sin_addr); // return the IP
		// printf("%s\n", some_addr); // prints "10.0.0.1"
		int status = sendto(s, &send_packet, sizeof(send_packet), 0, (const struct sockaddr *) &dest_info,  sizeof(dest_info));
		if(status < 0)
			perror("socket");
		// cout << "Send status from Data thread." << status << endl;

		//Send to next hop for the dest

	}
else if (entry_received.type == 3) // add link msg
    {
		cout << "Creating Link!" << endl;
		
		for (int i = 0; i < totalNodes; i++) {
			Node srcNode;
			Node destNode;
			// Get destination node
			for (int j = 0; j < totalNodes; j++) {
				if (nodeList[j].id = entry_received.dest) {
					destNode = nodeList[j];
				}
			}
			// Add destination node as neighbor to source node
			if (nodeList[i].id = entry_received.src) {
				srcNode = nodeList[i];
				nodeList[i].neighbors.insert(std::make_pair(entry_received.dest, destNode));
				neigbours.insert(std::make_pair(entry_received.dest, destNode));
			}
			// Add source node as neighbor to destination node
			for (int k = 0; k < totalNodes; k++) {
				if (nodeList[k].id == entry_received.dest) {
					nodeList[k].neighbors.insert(std::make_pair(entry_received.src, srcNode));
				}
			}
		}
		
	}

	else //remove link
	{
		cout << "Recived type 4" <<endl;

		for (int i = 0; i < totalNodes; i++) {
			map<int, Node>::iterator it;
			Node srcNode;
			Node destNode;
			
			for (int j = 0; j < totalNodes; j++) {
				// Get destination node and remove source node from its neighbors
				if (nodeList[j].id = entry_received.dest) {
					destNode = nodeList[j];
					it = nodeList[j].neighbors.find(entry_received.src);
					nodeList[j].neighbors.erase(it);
				}
				// Get source node and remove destination node from its neighbors
				if (nodeList[j].id = entry_received.src) {
					srcNode = nodeList[j];
					it = nodeList[j].neighbors.find(entry_received.dest);
					nodeList[j].neighbors.erase(it);
					neigbours.erase(it);
				}
			}
		}
	}

}
	return 0;
		
}

int send_receive(int s, int my_node_id, std::map<int, Node>& neigbours, std::map<int, VectorEntry>& rtable)
{	
	struct timeval read_timeout;

	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 10;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	std::map <int, VectorEntry> :: iterator itr_rtable;
	std::map <int, Node> :: iterator itr_n;
	struct sockaddr_in servaddr;

	fd_set read, r_temp;
	socklen_t from_len = sizeof(struct sockaddr);
	struct TableEntry entry_received;

	struct sockaddr_in serv, received_from;
	struct timeval tv, tv1;

	FD_ZERO(&read);
	FD_ZERO(&r_temp);

	FD_SET(s, &read);

	// tv.tv_sec = 5;
	// tv.tv_usec = 0;
		while(1)
		{

		tv.tv_sec = 5;
		tv.tv_usec = 0;
		FD_ZERO(&r_temp);

		
		r_temp = read;
		memcpy(&tv1, &tv, sizeof(tv));

		select(s+1, &r_temp, NULL, NULL, &tv); // This will either timeout or return
		
		if (FD_ISSET(s, &r_temp)) // above  returned
		{	while(1)
			{
			int size = recvfrom(s, &entry_received, sizeof(entry_received), 0, (struct sockaddr*) &received_from, &from_len);
			// cout << size << endl;
			if(size<0)
				break;

			// cout << "Size of recvfrom packet " << size <<endl;
			// std::cout << entry_received.sender_nodeID << std::endl;
			//Update routing table. Reconstruct packet ro TableEntry Format
			VectorEntry vecEntry = {entry_received.destination, entry_received.next_hop, entry_received.distance};
			// Pass received data to update - 
			int node_rec_from;
			for (int i = 0; i < totalNodes; i++) {
				if (inet_addr(nodeList[i].ip) == received_from.sin_addr.s_addr) {
					if (u_short(nodeList[i].controlPort) == received_from.sin_port) {
						node_rec_from = nodeList[i].id;
						
					}
				}
			}
		

			// cout << "Recived 1 vector from " << node_rec_from <<endl;
			update(rtable, vecEntry, node_rec_from);
			// cout << "Size of table: " << rtable.size() << endl;
			// std::cout << "received From" << received_from.sin_addr.s_addr << std::endl;
			}	
		}
		// cout << "total nodes " << totalNodes << endl;
		usleep(1000000);
		
		// for (itr_rtable = neigbours.begin(); itr_rtable != neigbours.end(); ++itr_rtable) 
		// {
		// 	cout << "Key - " << itr_n -> rtable << endl;
		// 	cout <<"Value - " << itr_rtable ->second <<endl;
		// }
		for (itr_n = neigbours.begin(); itr_n != neigbours.end(); ++itr_n) //loop though the neigbours 
    	{ 
  //  				loop through my vector entries
    		for (itr_rtable = rtable.begin(); itr_rtable != rtable.end(); ++itr_rtable) 
    		 	{ 
    				servaddr.sin_family = AF_INET;
    				servaddr.sin_port = itr_n->second.controlPort; 
    				inet_aton(itr_n->second.ip, &servaddr.sin_addr);
    	// 			std::cout<< itr_n->second.controlPort<<" " << std::endl;
    	// 			std::cout<< itr_n->second.ip<<" " << std::endl;

    				char* some_addr;
    				// std::cout << "Sending my vector to " << itr_n->first <<std::endl;
					// some_addr = inet_ntoa(servaddr.sin_addr); // return the IP
					// printf("%s", some_addr); // prints "10.0.0.1"

    				// std::cout<< servaddr.sin_port<<"\n" << std::endl;
    				TableEntry entry = {itr_rtable -> second.destination ,itr_rtable -> second.next_hop, itr_rtable -> second.distance, my_node_id};
    				// std::cout << "SENDING"<<std::endl;
    				int k = sendto(s, &entry, sizeof(entry), 0, (struct sockaddr *) &servaddr,  sizeof(servaddr)); 
    				// cout << "Key : " << itr_rtable -> first << endl;
    				// cout <<"Value : " << itr_rtable-> second.destination << itr_rtable-> second.next_hop << itr_rtable->second.distance<<endl;

    				// std::cout << "Sendto status"<< k << endl;
    			} 
		}
	}
return 0;
}

map<int, Node> getNeighbors(int nodeId) {
	map<int, Node> n;
	for (int i = 0; i < totalNodes; i++) {
		if (nodeList[i].id == nodeId) {
			cout << "Current Node is Node" << nodeId << " with the following neighbors: " << endl;
			for (auto& x: nodeList[i].neighbors) {
				cout << "Neighbor #: " << x.first+1 << " has node ID " << x.second.id << " with IP address: " << x.second.ip <<"  " <<  x.second.controlPort <<endl;
				n.insert(std::make_pair(x.second.id, x.second));
			}
			return n;
		}
	}
}


// int update(std::map<int, VectorEntry>& rtable, VectorEntry entry, int node_received_from);

int setupserver(u_short port, char* ip)
{
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET; 
    servaddr.sin_port = port; 
    inet_aton(ip, &servaddr.sin_addr);

    int k = bind(s, (const struct sockaddr*)&servaddr,sizeof(servaddr));
    if(k == -1)
    {
    	std::cout << "Bind err" << std::endl;
    	exit(1);
    }

    return s;
}

int main(int argc, char *argv[])  { 
	
	// std :: cout << "Hello!!" << endl;
	struct sockaddr_in serv,serv1, received_from;
	// string my_hostname;
	int my_cport, my_dport;
	 char* my_ip;

	// if (argc < 3) {
	// 	cout << "Please specify a <config.txt> file followed by a <NodeID>" << endl;
	// 	return 0;
	// }
	string config = "conf.txt";
	int nodeIn = atoi(argv[1]);
	/* Storing entire config to vector, 
	then parsing to create an array of nodes*/
	string line;
	vector<string> inputLines;
	totalNodes = 0;
	ifstream configFile(config);
	if (configFile.good()) {
		while (getline(configFile, line)) {
			inputLines.push_back(line);
		}
	}
	nodeList = new Node[inputLines.size()];
	for (int i = 0; i < inputLines.size(); i++) {
		Node node;
		struct in_addr **addr_list;
		struct hostent *host;
		//node.neighborIDs = new int[inputLines.size()];
		int numNeighbors = 0;
		stringstream str(inputLines[i]);
		str >> node.id;
		str >> node.hostname;
		str >> node.controlPort;
		str >> node.dataPort;
		while (!str.eof()) {
			int neighborId;
			str >> neighborId;
			node.neighborIDs.push_back(neighborId);
			numNeighbors++;
		}
		char charHostName[node.hostname.length()+1];
		strcpy(charHostName, node.hostname.c_str());
		host = gethostbyname(charHostName);
		node.addr_list = (struct in_addr**)host->h_addr_list;
		for (int i = 0; node.addr_list[i] != NULL; i++) {
			strcpy(node.ip, inet_ntoa(*node.addr_list[i]));
		}

		node.numNeighbors = numNeighbors;
		nodeList[i] = node;
		// Technically totalNodes == inputLines.size() == # of nodes in nodeList
		totalNodes++;	
	}
	
	for (int i = 0; i < inputLines.size(); i++) {
		if (nodeList[i].id == nodeIn) {
			my_ip = (char*)malloc((strlen(nodeList[i].ip)+1)*sizeof(char));
			strcpy(my_ip, nodeList[i].ip);
			my_dport = nodeList[i].dataPort;
			my_cport = nodeList[i].controlPort;
			for (int j = 0; j < nodeList[i].numNeighbors; j++) {
				for (int k = 0; k < inputLines.size(); k++) {
					if (nodeList[k].id == nodeList[i].neighborIDs.at(j)) {
						nodeList[i].neighbors[j] = nodeList[k];
					}
				}
			}
		}
	}

	std::map<int, Node> neigbours = getNeighbors(nodeIn);

	std::map <int, Node> :: iterator itr_n;


	

	//Accept CMD Parameters and get nodeID
	//Parse and get my neigbour map from config file
	// int prt = atoi(argv[2]);
	// int dprt = atoi(argv[3]);


    std::map<int, VectorEntry> entryTable;
	VectorEntry initialEntry = {nodeIn, -1, 0};
	entryTable.insert(std::make_pair(nodeIn, initialEntry));
	cout << my_cport << " " << my_dport;


	// int data = setupserver(serv1, u_short(atoi(argv[3]))); //data
	int s = setupserver((u_short)my_cport, my_ip);
	int ds = setupserver((u_short)my_dport, my_ip);
	std::thread control_thr(send_receive, s, nodeIn, std::ref(neigbours), std::ref(entryTable));
	std::cout << "Control Thread Started at" << (u_short)my_cport << std::endl;

	std::thread data_thr(data_thread, ds, nodeIn, std::ref(neigbours), std::ref(entryTable));
	std::cout << "Data Thread Started at " <<(u_short)my_dport <<std::endl;
	control_thr.join();
	data_thr.join();

	return 0;
}