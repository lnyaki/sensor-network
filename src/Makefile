CONTIKI_PROJECT = sensor-new

TARGET_LIBFILES += messages/sendMessage.h
TARGET_LIBFILES += messages/receiveMessage.h

HEADERS = sensor.h messages/receiveMessage.h messages/sendMessage.h
OBJECTS = sensor-new.o

all: $(CONTIKI_PROJECT)

#UIP_CONF_IPV6=1

CONTIKI_WITH_RIME = 1
CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
