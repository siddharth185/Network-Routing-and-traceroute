# Network-Routing-and-traceroute

Nodes on this virtual network use distance vector routing for determining the shortest path to other nodes in the
network.
Each node periodically sends its distance vector to all its directly connected neighbors. For example, node 1
will send its distance vector to nodes 2, 4, and 5. Each distance vector entry contains (Destination, Next
hop, Distance) - to send a packet to destination Destination, the packet should first be forwarded to next
hop Next hop, and the total number of hops to the destination is Distance.
Each node also listens for incoming distance vectors sent by its neighbors. Whenever a new distance vector
is received, the node will follow the distance vector routing algorithm.

Routing messages, i.e., distance vectors, are exchanged using the node’s control port 


Data thread is used to forward non control messages.


Usage - 

Fill in the nodes and their data and control port along with the neighbour nodes in conf.txt


Run controlclient and to perform trace route between node 1 and 2

routetrace 1 2



