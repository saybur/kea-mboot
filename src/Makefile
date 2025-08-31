# modified from https://github.com/zorun/kea-hook-runscript (MPL2)

KEA_MSG_COMPILER ?= kea-msg-compiler
KEA_INCLUDE ?= /usr/include/kea
KEA_LIB ?= /usr/lib
KEA_USER_HOOKS ?= $(shell /usr/sbin/kea-dhcp4 -W | grep Hooks\ directory | awk '{print $$3}')

SRCS = callouts.cc log.cc messages.cc version.cc
OBJS = $(SRCS:.cc=.o)
TARGET = mboot-hook

CXXFLAGS = -I $(KEA_INCLUDE) -fPIC -Wall -std=c++11
LDFLAGS = -L $(KEA_LIB) -shared -lkea-dhcpsrv -lkea-dhcp++ -lkea-hooks -lkea-log -lkea-util -lkea-exceptions

$(TARGET).so: $(OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $(OBJS)

%.o: %.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o
	rm -f $(TARGET).so

install:
	mkdir -p $(KEA_USER_HOOKS) && cp $(TARGET).so $(KEA_USER_HOOKS)/.
