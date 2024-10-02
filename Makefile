# modified from https://github.com/zorun/kea-hook-runscript (MPL2)

KEA_MSG_COMPILER ?= kea-msg-compiler
KEA_INCLUDE ?= /usr/include/kea
KEA_LIB ?= /usr/lib
KEA_USER_HOOKS ?= /usr/local/lib/kea/hooks

SRCS = src/callouts.cc src/log.cc src/messages.cc src/version.cc
OBJS = $(SRCS:.cc=.o)
TARGET = mboot-hook

CXXFLAGS = -I $(KEA_INCLUDE) -fPIC -Wall -std=c++11
LDFLAGS = -L $(KEA_LIB) -shared -lkea-dhcpsrv -lkea-dhcp++ -lkea-hooks -lkea-log -lkea-util -lkea-exceptions

$(TARGET).so: $(OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $(OBJS)

%.o: %.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f src/*.o
	rm -f $(TARGET).so

install:
	mkdir -p $(KEA_USER_HOOKS) && cp $(TARGET).so $(KEA_USER_HOOKS)/.
