CMD_CC ?= gcc
CFLAGS ?= -O2
LDFLAGS ?=

all: gkrellm-gkfreq.so

gkrellm-gkfreq.o: gkrellm-gkfreq.c
	$(CMD_CC) -fPIC $(CFLAGS) -Wall `pkg-config gtk+-2.0 --cflags` -c gkrellm-gkfreq.c

gkrellm-gkfreq.so: gkrellm-gkfreq.o
	$(CMD_CC) -shared $(LDFLAGS) -Wall -o gkrellm-gkfreq.so gkrellm-gkfreq.o
	@echo " "
	@echo "    Compilation done!"
	@echo " "
	@echo "    If you want to try the plugin before installing you can run"
	@echo "        gkrellm -p gkrellm-gkfreq.so"
	@echo " "
	@echo "    Install in either /usr/lib/gkrellm2/plugins/ by running"
	@echo "        sudo make install"
	@echo "    or in /usr/local/lib/gkrellm2/plugins/ by running"
	@echo "        sudo make install-local"
	@echo "    or in ~/.gkrellm2/plugins by running"
	@echo "        make install-home"
	@echo " "


clean:
	rm -rf *.o *.so *~

install:
	cp gkrellm-gkfreq.so /usr/lib/gkrellm2/plugins/

install-local:
	cp gkrellm-gkfreq.so /usr/local/lib/gkrellm2/plugins/

install-home:
	cp gkrellm-gkfreq.so ~/.gkrellm2/plugins/
