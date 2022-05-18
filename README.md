# Multi-Threaded-Webserver
UGA CSCI4730 Project detailed around effective use of semaphores and mutex locks for safe access of critical code sections between multiple threads.
Uses a Thread-Pool model using a predetermined number of threads to service requests.
Heavily related to the Bounded-Buffer Problem

webserver.c   |   usage: ./webserver 4000 10 (port 4000, create 10 child threads)
Starts a single listener thread (producer) and multiple child threads (consumer). The listener receives requests and forwards them to the consumer threads that utilize mutex locks to smartly process the requests without simultaneously accessing the array containing requests.

client.c   |   usage: ./client 127.0.0.1 4004 10 (IP 127.0.0.1, port 4000, optionally creates 10 child threads)
Simulates client connections for webserver.c to handle.
