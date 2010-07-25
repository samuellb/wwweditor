
all: wwwproject

CFLAGS += -Wall -Wextra -g --std=c99 `pkg-config --cflags 'gtk+-2.0 >= 2.16' webkit-1.0` -DGTK_DISABLE_DEPRECATED=1 -DGDK_DISABLE_DEPRECATED=1 -DG_DISABLE_DEPRECATED=1 -DGSEAL_ENABLE
LDFLAGS += -Wl,--as-needed
LIBS += `pkg-config --libs 'gtk+-2.0 >= 2.16' webkit-1.0`

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

domnotify.c: domnotify.js
	echo "const char *domnotify_js = " > $@.tmp
	(yui-compressor --charset UTF-8 domnotify.js || cat domnotify.js) | sed -E 's/(["\\])/\\\1/g' | sed -E 's/(.*)/"\1\\n"/' >> $@.tmp
	echo ";" >> $@.tmp
	mv -f $@.tmp $@

OBJECTS := domnotify.o main.o webview_common.o webview_webkit.o

main.o: webview.h
webview_common.o: webview.h webview_private.h
webview_webkit.o: webview.h webview_private.h

wwwproject: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@


.PHONY: all clean install uninstall
clean:
	rm -f $(OBJECTS) domnotify.c wwwproject


