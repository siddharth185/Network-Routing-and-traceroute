// TODO
// 1)Send vectors to neigbours, need neigbours list for that
// 2)Map Address to NodeID inorder to update rtable
// 3)Mutex lock the rtable before updating
// #include <map>
// #include <vector>

extern int totalNodes;




// int data_thread(int data_socket, int my_node_id, std::map<int, Node>& neigbours, std::map<int, Node>& rtable, Node* NodeList, int total_nodes){

// 		std::map<int, int> TTLmap; // Remembers ttl values of packet id of current . key - packet_id, value - TTL
// 		int curr_packet_id = 0;

// 		socklen_t from_len = sizeof(struct sockaddr);
// 		struct data_packet entry_recieved;
// 		struct sockaddr_in received_from;
		
// 		recvfrom(data_socket, &entry_recieved, sizeof(entry_recieved), 0, (struct sockaddr*) &received_from, &from_len);
// 		if (entry_recieved.type == 1) // routetrace msg
// 		{
// 			if (entry_recieved.trig_by == my_node_id) // trig by me
// 			{
// 				std::cout << "Routetrace : " << entry_recieved.src << std:endl;
// 				TTLmap[entry_recieved.packet_id]++; // increase TTL of that  
// 				entry_recieved.ttl = TTLmap.at(entry_recieved.src) // modify the recieved packet ttl

// 				//TODO - Look at Rtable and forward to next hop
// 			}
// 			else
// 			{
// 				if(entry_recieved.ttl == 0) //send back to node which trig TODO - Modify to travel through hops rather than direct
// 				{
// 					// entry_recieved.ttl = 15;
// 					// int source = entry_recieved.source 
// 					// entry_recieved.source = entry_recieved.dest
// 					// entry_recieved.dest = source
// 					//send package

// 					//get node info
// 					sockaddr_in dest_info;
// 					memset(&dest_info, 0, sizeof(dest_info));


// 					for (int i = 0; i < totalNodes; i++) //iterate through the nodelist to get info of trig_by
// 						if (nodeList[i].id == entry_recieved.trig_by)
// 						{
// 							dest_info.sin_family = AF_INET;
// 							dest_info.sin_port = u_short(nodeList[i].dataPort);
// 							dest_info.sin_addr.s_addr = inet_aton(nodeList[i].ip, &dest_info.sin_addr);
// 						}


// 					sendto(s, &entry_recieved, sizeof(entry_recieved), 0, (struct sockaddr *) &dest_info,  sizeof(dest_info)); 
// 					//Send package

// 				}
// 				else
// 				{
// 					entry_recieved.ttl -=1;
// 					//TODO - See Rtable and forward to next hop for this destination
// 				}
// 			}
// 		}

// 	else // type 2 msg, recieved from client
// 	{	
// 		packet_id+=1 ;
// 		int new_packet_id = packet_id;
// 		sockaddr_in dest_info;
// 		struct data_packet send_packet;
		
// 		//Trigger new route trace to dest, send a type 1 message to src
// 		// 
// 		TTLmap[new_packet_id]++; //set packet_id value to 0

// 		//build data_packet
// 		send_packet.type = 1;
// 		send_packet.trig_by = entry_recieved.src;
// 		send_packet.src =  entry_recieved.src;
// 		send_packet.dest = entry_recieved.dest;
// 		send_packet.ttl = 0;

// 		//TODO - Lookup nexthop for this dest in rtable and send packet
// 		// dest_info.sin_family = AF_INET;
// 		// dest_info.sin_port = u_short(nodeList[i].dataPort);
// 		// dest_info.sin_addr.s_addr = inet_aton(nodeList[i].ip, &dest_info.sin_addr);


// 		//Send to next hop for the dest

// 	}
		
// }
