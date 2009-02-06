# Makefile for the SPriG library

include Makefile.conf

CFLAGS += $(SPRIG_CFLAGS) -fPIC
LIBS = $(SPRIG_LIBS)

SPRIG_VER = 1
SPRIG_VER_MINOR = 0
SPRIG_VER_BUGFIX = 0

OBJECTS=SPG_surface.o SPG_primitives.o SPG_polygon.o SPG_rotation.o SPG_misc.o

all:	config $(OBJECTS) 
	@ar rsc libsprig.a $(OBJECTS)

$(OBJECTS):	%.o:%.c   #Each object depends on its .cpp and .h file
	$(CXX) $(CFLAGS) -c $<

shared: all
	$(CXX) $(CFLAGS) -Wl,-soname,libsprig.so.$(SPRIG_VER) -fpic -fPIC -shared -o libsprig.so $(OBJECTS) $(LIBS)

shared-strip:	shared
	@strip libsprig.so

# Building a dll... I have no idea how to do this, but it should be something like below.
dll:	config $(OBJECTS)
	dlltool --output-def sprig.def $(OBJECTS)
	dllwrap --driver-name $(CXX) -o sprig.dll --def sprig.def --output-lib libsprig.a --dllname sprig.dll $(OBJECTS) $(LIBS)

dll-strip:	dll
	@strip sprig.dll

clean:
	@rm -f *.o *.so *.a *.dll *.def

config:

ifneq ($(QUIET),y)
	@echo "== SPriG v$(SPRIG_VER).$(SPRIG_VER_MINOR).$(SPRIG_VER_BUGFIX)"
endif

install:	shared
	@mkdir -p $(PREFIX_H)
	install -c -m 644 sprig.h $(PREFIX_H)
	install -c -m 644 sprig_inline.h $(PREFIX_H)
	@mkdir -p $(PREFIX)/lib
	install -c -m 644 libsprig.a $(PREFIX)/lib
	install -c libsprig.so $(PREFIX)/lib/libsprig.so.$(SPRIG_VER).$(SPRIG_VER_MINOR).$(SPRIG_VER_BUGFIX)
	@cd $(PREFIX)/lib;\
	ln -sf libsprig.so.$(SPRIG_VER).$(SPRIG_VER_MINOR).$(SPRIG_VER_BUGFIX) libsprig.so.$(SPRIG_VER);\
	ln -sf libsprig.so.$(SPRIG_VER) libsprig.so;
	@echo
	@echo "** Headerfiles installed in $(PREFIX_H)"
	@echo "** Library files installed in $(PREFIX)/lib"
