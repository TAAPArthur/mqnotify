
all: mqnotify notify-send

install: mqnotify notify-send
	install -t $(DESTDIR)/usr/bin/ $^

mqnotify: mqserver.o | config.h
	$(CC) $(CFLAGS) -o $@ $^ -lrt

notify-send: notify-send.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

config.h:
	cp config.def.h config.h

clean:
	rm -f  mqnotify notify-send *.o
