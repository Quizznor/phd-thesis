ifdef ARM
   CC=arm-xilinx-linux-gnueabi-gcc
endif

UUB_DAQ_BASE_DIR=../../../
UUBLIB= $(UUB_DAQ_BASE_DIR)/lib/Config/config.c \
        $(UUB_DAQ_BASE_DIR)/lib/Util/prtlog.c \
        $(UUB_DAQ_BASE_DIR)/lib/Dmcompatlib/dmcompatlib.c


a.out: t.c muonAcqStore.c
	gcc t.c muonAcqStore.c -I. -I../../../include

run: a.out
	rm muonAcqStore.out;
	echo 3 > muonAcqStore.start;\
	a.out

acq: muonAcqStore.c muon_acquire.c muoncalib.c muon_read.c
	$(CC) muonAcqStore.c muon_acquire.c muoncalib.c muon_read.c -I. -I$(UUB_DAQ_BASE_DIR)/include -I$(UUB_DAQ_BASE_DIR)/fpga_link/include/ -I$(UUB_DAQ_BASE_DIR)/lib/FeLib/real/ $(UUBLIB) -lrt -lm -o $@


clean: ;
	rm a.out acq 
