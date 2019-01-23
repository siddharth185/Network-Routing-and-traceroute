all: node controlClient

node:	node.o
	g++ -o node node.cpp -pthread

controlClient:	controlClient.o
	g++ controlClient.o -o controlClient

clean:
	rm -f *.o