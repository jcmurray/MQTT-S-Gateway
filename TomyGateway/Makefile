PROGNAME := TomyGateway
SRCDIR := src
SUBDIR := src/libmq

SRCS := $(SRCDIR)/A_ProgramStructure.cpp \
$(SRCDIR)/BrokerRecvTask.cpp \
$(SRCDIR)/BrokerSendTask.cpp \
$(SRCDIR)/ClientRecvTask.cpp \
$(SRCDIR)/ClientSendTask.cpp \
$(SRCDIR)/GatewayControlTask.cpp \
$(SRCDIR)/GatewayResourcesProvider.cpp \
$(SUBDIR)/ProcessFramework.cpp \
$(SUBDIR)/Messages.cpp \
$(SUBDIR)/Socket.cpp \
$(SUBDIR)/Topics.cpp \
$(SUBDIR)/ZBeeStack.cpp 

CXX := g++
CPPFLAGS += 
DEFS :=
LDFLAGS += 
LIBS +=
LDADD := -lpthread -lrt

CXXFLAGS := -Wall -Wextra -O3

OUTDIR := Build

PROG := $(OUTDIR)/$(PROGNAME)
OBJS := $(SRCS:%.cpp=$(OUTDIR)/%.o)
DEPS := $(SRCS:%.cpp=$(OUTDIR)/%.d)

.PHONY: install clean distclean

all: $(PROG)

-include $(DEPS)

$(PROG): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDADD)

$(OUTDIR)/%.o:%.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEFS) -o $@ -c -MMD -MP -MF $(@:%.o=%.d) $<

clean:
	rm -rf $(OUTDIR)

distclean:
	rm -rf Build
