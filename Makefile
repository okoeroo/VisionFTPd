RM = /bin/rm
CAT = /bin/cat
CC = gcc
UNAME = uname

#   -g -DDEBUG \

CFLAGS = \
   -g -DDEBUG \
   -Wall \
    -Wcast-qual -Wchar-subscripts -Winline \
    -Wmissing-prototypes -Wnested-externs -Wpointer-arith \
    -Wredundant-decls -Wshadow -Wstrict-prototypes \
    -Wpointer-arith -Wno-long-long \
    -Wcomment \
    -O3 -Wuninitialized

CPPFLAGS = \
	-I/usr/include/ \
	-Isrc/logging/ \
	-Iinclude/ \
	-Isrc/network/ \
	-Isrc/cmdftpd/ \
	-I. \

LFLAGS = -lssl -lcrypto


SYS = -s
MAH = -m

BIN   =  ./bin/visionftpd

OBJS  = \
	src/logging/scar_log.o \
	src/network/net_common.o \
	src/network/net_client.o \
	src/network/net_server.o \
	src/network/net_threader.o \
	src/unsigned_string.o \
	src/daemonize.o \
	src/cmdftpd/ftpd.o \
	src/cmdftpd/commander.o \
	src/irandom.o \
	src/main.o


all: $(BIN)


run: $(BIN)
	$(BIN); echo $$?

kill:
	killall `basename $(BIN)`

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJS) $(LFLAGS)


# $(OBJS): 
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) -f $(BIN) $(OBJS) $(BINCLIENT) $(OBJSCLIENT) *~

distclean: clean
