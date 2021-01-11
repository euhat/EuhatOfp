COMPILER=gcc
DPDK_PATH=/home/eu/work/dpdk/dpdk-19.11/x86_64-native-linuxapp-gcc
DPDK_CFLAGS=-I$(DPDK_PATH)/include
DPDK_LDFLAGS=-L$(DPDK_PATH)/lib -Wl,--whole-archive -Wl,-ldpdk -Wl,--no-whole-archive -lnuma -lm -Wl,-lpcap
ODP_DPDK_PATH=/home/eu/work/dpdk/odp-dpdk/installed
ODP_DPDK_CFLAGS=-I$(ODP_DPDK_PATH)/include
ODP_DPDK_LDFLAGS=-L$(ODP_DPDK_PATH)/lib $(ODP_DPDK_PATH)/lib/libodp-dpdk.a $(ODP_DPDK_PATH)/lib/libodphelper.a
OFP_PATH=/home/eu/work/dpdk/ofp/installed
OFP_CFLAGS=-I$(OFP_PATH)/include
OFP_LDFLAGS=-L$(OFP_PATH)/lib -lofp

CFLAGS=-O0 -g $(OFP_CFLAGS) $(ODP_DPDK_CFLAGS) $(DPDK_CFLAGS)
LDFLAGS=$(OFP_LDFLAGS) $(ODP_DPDK_LDFLAGS) $(DPDK_LDFLAGS) -lconfig -lcrypto -lpthread -latomic -ldl

SOURCES_HS=$(wildcard ../hs/*.c)
SOURCES_TEST=$(wildcard *.c)
 
OBJS_HS=$(SOURCES_HS:../hs/%.c=release/hs/%.o)
OBJS_TEST=${SOURCES_TEST:%.c=release/%.o}
 
TARGET=release/test
 
#TEST0=./release/test 0 --proc-type=primary --socket-mem=128 --file-prefix=test/.test0_config 
TEST0=./release/test 0

all: before $(TARGET)
 
before:
	@test -d release || mkdir -p release
	@test -d release/hs || mkdir -p release/hs
 
$(OBJS_HS): release/hs/%.o: ../hs/%.c
	$(COMPILER) -c $(CFLAGS) $< -o $@
 
$(OBJS_TEST): release/%.o: %.c
	$(COMPILER) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS_HS) $(OBJS_TEST)
	$(COMPILER) -o $@ $^ $(LDFLAGS)
 
clean:
	rm -Rf release
	rm -Rf log

dbg:
	gdb --args $(TEST0)

run:
	$(TEST0)

l:
	for i in `ls log/*`; do echo $$i; cat $$i; done
