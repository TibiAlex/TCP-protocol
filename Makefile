PORT = 8080
IP_SERVER = 127.0.0.1
ID_CLIENT = mujan123

all: server subscriber 

server:
	g++ server.cpp -o server -Wall

subscriber:
	g++ subscriber.cpp -o subscriber -Wall

run_server:
	./server ${PORT}

run_client:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber

