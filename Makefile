all: server client

server: server.c
	gcc $$(pkg-config --libs --cflags glib-2.0) \
		$$(pkg-config --libs --cflags gobject-2.0) \
		$$(pkg-config --libs --cflags oren-1.0)-o $@ $^ \
		-lglib-2.0 -lgobject-2.0 -loren-1.0

client: client.c
	gcc $$(pkg-config --libs --cflags glib-2.0) \
		$$(pkg-config --libs --cflags gobject-2.0) \
		$$(pkg-config --libs --cflags oren-1.0)-o $@ $^ \
		-lglib-2.0 -lgobject-2.0 -loren-1.0