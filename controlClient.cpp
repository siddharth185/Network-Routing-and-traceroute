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
#include <sstream>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <netdb.h>


using namespace std;
struct packet {
	uint8_t sourceId;
	uint8_t destinationId;
	uint8_t packetId;
	uint8_t timeToLive;
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

int main(int argc, char* argv[]) {
	// if (argc != 5) {
	// 	cout << "Command line argument format: <action> <node1> <node2> <conf>" << endl;
	// 	exit(1);
	// }

	int totalNodes;
	string action;
	int nodeOneId;
	int nodeTwoId;
	string config = argv[1];
	uint8_t curr_id = 0;

	string line;
	vector<string> inputLines;
	totalNodes = 0;
	ifstream configFile(config);
	if (configFile.good()) {
		while (getline(configFile, line)) {
			inputLines.push_back(line);
		}
	}
	Node* nodeList = new Node[inputLines.size()];
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
		// cout << node.id << node.dataPort;
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
		totalNodes++;
	}
	// cout << totalNodes << endl;



	// Identifies and initializes nodes from argv
	Node nodeOne;
	Node nodeTwo;
	while (1){
		cin >> action >> nodeOneId >> nodeTwoId;
		cout << "Packet Id: " << unsigned(curr_id) << "\n";
	// cout << totalNodes << endl;
	for (int i = 0; i < totalNodes; i++) {
		if (nodeList[i].id == nodeOneId) {
			nodeOne = nodeList[i];
		}
		if (nodeList[i].id == nodeTwoId) {
			nodeTwo = nodeList[i];
		}
	}

	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));

	if (action == "routetrace") {
		/* Ask node 1 to create route trace packets to node 2, node 1 prints out ids
		of all nodes responding on the path */
		struct data_packet dp = {2, // Type 2
					uint8_t(nodeOneId), // src
					uint8_t(nodeOneId), // triggered by
					uint8_t(nodeTwoId), // dest
					curr_id, // packet id
					0, // ttl
					0,
					0,
					0, // rttl
					uint8_t(nodeTwoId) // responder
					};
			servaddr.sin_family = AF_INET; 
    		
			// cout << totalNodes<<endl;
			for (int i = 0; i < totalNodes; i++) {
				// cout << nodeList[i].id << " " << nodeList[i].dataPort << endl;
				// cout << nodeList[i].id << " " << nodeList[i].ip << endl;
				}



			// char* prt;
    		for (int i = 0; i < totalNodes; i++) {
				if (nodeList[i].id == nodeOneId) {
					servaddr.sin_port = u_short(nodeList[i].dataPort); 
    				inet_aton(nodeList[i].ip, &servaddr.sin_addr);
    				break;				
				}
			}

		// servaddr.sin_port = u_short(nodeOne.dataPort); 
  //   	inet_aton(nodeOne.ip, &servaddr.sin_addr);
		// cout << "Sending to" << prt << endl;
		sendto(s, &dp, sizeof(dp), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
		cout << "Routetrace sent!" << endl;
	}
	else if(action == "create-link") {
		/* Establish link with node 1 and 2, exchange distance vectors with each other
		and update their routing tables*/ 
		struct data_packet dp = {3, // Type 3
					uint8_t(nodeOneId), // src
					uint8_t(nodeOneId), // triggered by
					uint8_t(nodeTwoId), // dest
					curr_id, // packet id
					0, // ttl
					0,
					0,
					0, // rttl
					uint8_t(nodeTwoId) // responder
					};
		    for (int i = 0; i < totalNodes; i++) {
				if (nodeList[i].id == nodeOneId) {
					servaddr.sin_port = u_short(nodeList[i].dataPort); 
    				inet_aton(nodeList[i].ip, &servaddr.sin_addr);
    				break;				
				}
			}
		sendto(s, &dp, sizeof(dp), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));

		struct data_packet dp1 = {3, // Type 3
					uint8_t(nodeTwoId), // src
					uint8_t(nodeTwoId), // triggered by
					uint8_t(nodeOneId), // dest
					curr_id, // packet id
					0, // ttl
					0,
					0,
					0, // rttl
					uint8_t(nodeOneId) // responder
					};
		    		for (int i = 0; i < totalNodes; i++) {
				if (nodeList[i].id == nodeOneId) {
					servaddr.sin_port = u_short(nodeList[i].dataPort); 
    				inet_aton(nodeList[i].ip, &servaddr.sin_addr);
    				break;				
				}
			}
		sendto(s, &dp1, sizeof(dp1), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
	}
	else if (action == "remove-link") {
		/* Remove link between node 1 and 2, update their routing tables and sends
		them to their neighbors */
		struct data_packet dp = {4, // Type 4
					uint8_t(nodeOneId), // src
					uint8_t(nodeOneId), // triggered by
					uint8_t(nodeTwoId), // dest
					curr_id, // packet id
					0, // ttl
					0,
					0,
					0, // rttl
					uint8_t(nodeTwoId) // responder
					};
		    		for (int i = 0; i < totalNodes; i++) {
				if (nodeList[i].id == nodeOneId) {
					servaddr.sin_port = u_short(nodeList[i].dataPort); 
    				inet_aton(nodeList[i].ip, &servaddr.sin_addr);
    				break;				
				}
			}
		sendto(s, &dp, sizeof(dp), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));

		struct data_packet dp1 = {4, // Type 4
					uint8_t(nodeTwoId), // src
					uint8_t(nodeTwoId), // triggered by
					uint8_t(nodeOneId), // dest
					curr_id, // packet id
					0, // ttl
					0,
					0,
					0, // rttl
					uint8_t(nodeOneId) // responder
					};
					
		    		for (int i = 0; i < totalNodes; i++) {
				if (nodeList[i].id == nodeOneId) {
					servaddr.sin_port = u_short(nodeList[i].dataPort); 
    				inet_aton(nodeList[i].ip, &servaddr.sin_addr);
    				break;				
				}
			}
		sendto(s, &dp1, sizeof(dp1), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
	}
	else {
		cout << "Command line argument <action> must be: routetrace, create-link, or remove-link" << endl;
	}
	curr_id+=1;
}
}