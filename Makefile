all: servermain serverA serverB client ./servermain ./serverA ./serverB ./client

servermain: servermain.cpp
	g++ -o servermain servermain.cpp

serverA: serverA.cpp
	g++ -o serverA serverA.cpp

serverB: serverB.cpp
	g++ -o serverB serverB.cpp

client: client.cpp
	g++ -o client client.cpp

clean:
	rm *.o serverA serverB servermain