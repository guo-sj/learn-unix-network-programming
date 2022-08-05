all:
	cc stream_server.c -o stream_server
	cc stream_client.c -o stream_client

clean:
	rm stream_server stream_client
