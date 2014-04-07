all: position_analyzer

%: %.c
	gcc `pkg-config cairo-xlib-xrender --cflags --libs` -pthread -lm $< -o $@
