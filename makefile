all:
	gcc -Wall -std=c99 lgo.c -o lgo -I. -L. -lraylib
	./lgo

web:
	emcc -Os -Wall lgo.c ./libraylib.web.a -o lgo.html -I. -s USE_GLFW=3 -s -DPLATFORM_WEB --preload-file assets
