TARGETS=gl.def glx.def egl.def

all: $(TARGETS)

%.def: %.xml
	./xml2def $? > $@

glx.def: glx.xml
	./xml2def $? | grep -v GetProcAddress > $@

egl.def: egl.xml
	./xml2def $? | grep -v eglGetProcAddress > $@

clean:
	rm *.def
