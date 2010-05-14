RM = /bin/rm
CAT = /bin/cat
CC = gcc
UNAME = uname

#   -g -DDEBUG \

CFLAGS = \
   -g -DDEBUG \
   -Wall \
   -ansi \
   -pedantic \
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
	-Isrc/transfer_manager/ \
	-Isrc/grandcentral/ \
	-Isrc/cmdftpd/ \
	-Isrc/vfs/ \
	-I. \

LFLAGS = -lssl -lcrypto -lpthread


SYS = -s
MAH = -m

BIN   =  ./bin/visionftpd
#CHROOT = /tmp/
CHROOT = /Users/okoeroo/dvl/clib/visionftpd

OBJS  = \
	src/logging/scar_log.o \
	src/network/net_common.o \
	src/network/net_client.o \
	src/network/net_server.o \
	src/network/net_threader.o \
	src/network/net_buffer.o \
	src/network/net_messenger.o \
	src/transfer_manager/trans_man.o \
	src/unsigned_string.o \
	src/daemonize.o \
	src/grandcentral/dispatcher.o \
	src/cmdftpd/ftpd.o \
	src/cmdftpd/commander.o \
	src/irandom.o \
	src/vfs/vfs_func.o \
	src/vfs/vfs.o \
	src/main.o


all: $(BIN)


run: $(BIN)
	$(BIN) --chroot $(CHROOT); echo $$?

kill:
	killall `basename $(BIN)`

$(BIN): $(OBJS) dirs
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJS) $(LFLAGS)

dirs:
	mkdir -p bin/ lib/

# $(OBJS): 
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) -f $(BIN) $(OBJS) $(BINCLIENT) $(OBJSCLIENT) *~

distclean: clean
