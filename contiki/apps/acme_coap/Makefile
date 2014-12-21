DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = acme_coap

TARGETDIRS += ../../platform

all: $(CONTIKI_PROJECT)

APPS += er-coap
APPS += rest-engine

CONTIKI = ../../../../contiki
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
