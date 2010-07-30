
all: wwwproject

CFLAGS += -Wall -Wextra -g --std=c99 `pkg-config --cflags 'gtk+-2.0 >= 2.16' webkit-1.0` -DGTK_DISABLE_DEPRECATED=1 -DGDK_DISABLE_DEPRECATED=1 -DG_DISABLE_DEPRECATED=1 -DGSEAL_ENABLE
LDFLAGS += -Wl,--as-needed
LIBS += `pkg-config --libs 'gtk+-2.0 >= 2.16' webkit-1.0`

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

editor_js.c: editor.js
	echo "const char *editor_js = " > $@.tmp
	(yui-compressor --charset UTF-8 editor.js || cat editor.js) | sed -E 's/(["\\])/\\\1/g' | sed -E 's/(.*)/"\1\\n"/' >> $@.tmp
	echo ";" >> $@.tmp
	mv -f $@.tmp $@

OBJECTS := editor_js.o main.o template.o tokenizer.o webview_common.o webview_webkit.o

main.o: webview.h
template.o: template.h tokenizer.h
tokenizer.o: tokenizer.h
webview_common.o: webview.h webview_private.h
webview_webkit.o: webview.h webview_private.h

wwwproject: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@


# Useful commands
html5enum:
	sort html5tags.txt | sed -r 's/(.*)/Tag_\1, /' | tr -d '\n\r' | fmt

html5strings:
	sort html5tags.txt | sed -r 's/(.*)/"\1", /' | tr -d '\n\r' | fmt

html5comments:
	sort html5tags.txt | sed -r 's/(.*)/\/\/ <\1>/'



.PHONY: all clean install uninstall
clean:
	rm -f $(OBJECTS) editor_js.c wwwproject


