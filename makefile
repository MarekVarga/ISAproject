# makefile for project for ISA course
# 
# author: MAREK VARGA
# login: xvarga14

CC = gcc
CFLAGS = -Wall -Wextra -pedantic
SERVER = server/isaserver
CLIENT = client/isaclient
TEST = test

all: $(SERVER) $(CLIENT) $(TEST) $(UTILS)

$(SERVER): $(SERVER).c
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).c

$(CLIENT): $(CLIENT).c
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).c

$(TEST): $(TEST).c
	$(CC) $(CFLAGS) -o $(TEST) $(TEST).c
