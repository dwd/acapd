RTOP=../../
MODULE=acapd-real
OBJDEPS=commands/authenticate commands/compress commands/deleteacl \
	commands/deletedsince commands/freecontext commands/getquota \
	commands/listrights commands/logout commands/myrights \
	commands/search commands/setacl commands/starttls \
	commands/store commands/updatecontext
LIBDEPS=server datastore utils threading stream
LIBDIRS=
EXTLIBS=ssl crypto expat sasl2 z
OUTPUT=exec
WHEN=all

SUBDIRS=utils threading commands datastore server stream meta

ifndef TOP
include $(RTOP)/bs/Make.local
endif
