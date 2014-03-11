all: position_analyzer

%: %.c
	gcc `pkg-config cairo-xlib-xrender --cflags --libs` -pthread -lm $< -o $@

test: position_analyzer
	cat ../../../surveyor/logs/logy_2013/finale/final_anglie | while read i; do echo "$i"; sleep 0.00001; done | ./position_analyzer
