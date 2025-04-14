
EXE=make_flash
EXE_GENVER=gen_softver

OBJS=make_flash.o md5.o md5_interface.o cJSON.o
OBJS_GENVER=gen_softver.o md5.o md5_interface.o cJSON.o

LDFLAGS += -lm
CFLAGS += -std=gnu99
CFLAGS += $(SMB_PRIV_COMMON_CFLAGS)

all: $(EXE) $(EXE_GENVER)
.PHONY: all

$(EXE):$(OBJS)
	$(CC) $^ -o $(EXE) $(LDFLAGS)

$(EXE_GENVER):$(OBJS_GENVER)
	$(CC) $^ -o $(EXE_GENVER) $(LDFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	@rm -rf $(EXE) $(EXE_GENVER) *.o
