
.PHONY: all clean distclean

OBJS = cmd.o command.o pixel_dtb.o protocol.o settings.o psi46test.o datastream.o analyzer.o profiler.o rpc.o rpc_calls.o usb.o rpc_error.o rs232.o iseg.o
# chipdatabase.o color.o defectlist.o error.o histo.o pixelmap.o prober.o ps.o scanner.o
# test_dig.o
# plot.o

ROOTCFLAGS = $(shell $(ROOTSYS)/bin/root-config --cflags)
# root C flags =  -pthread -m64 -I/home/pitzl/ROOT/armin/root-cern/include

ROOTLIBS   = $(shell $(ROOTSYS)/bin/root-config --libs)
ROOTGLIBS  = $(shell $(ROOTSYS)/bin/root-config --glibs) # with Gui

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CXXFLAGS = -g -Os -Wall $(ROOTCFLAGS) -I/usr/local/include -I/usr/X11/include
LDFLAGS = -lftd2xx -lreadline -L/usr/local/lib -L/usr/X11/lib -lX11 $(ROOTGLIBS)
endif

# -g for gdb
# -pg for gprof
# -Q for function statistics (verbose!)
# -fstack-usage (unrecognized)
# -fprofile-report (unrecognized)
# -fprofile-arcs (needs -lgcov)
# -fdump-translation-unit (.tu huge) 
# -fdump-class-hierarchy
# -fdump-tree-optimized
# -fdump-tree-cfg
# -fdump-tree-all (many files)

ifeq ($(UNAME), Linux)
CXXFLAGS = -g -pg -O2 -Wall -Wextra $(ROOTCFLAGS) -I/usr/local/include -I/usr/X11/include -pthread
LDFLAGS = -lftd2xx -lreadline -lgcov -L/usr/local/lib -pthread -lrt $(ROOTGLIBS) -L/usr/X11/lib -lX11
endif

# gcc -O2 -Wall -Wextra -I/home/pitzl/ROOT/armin/root-cern/include -fdump-class-hierarchy code.cpp

# PATTERN RULES:
obj/%.o : %.cpp  %.h
	@mkdir -p obj/linux
	@echo 'root C flags = ' $(ROOTCFLAGS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

#obj/%.d : %.cpp obj
#	@mkdir -p obj/linux
#	$(shell $(CXX) -MM $(CXXFLAGS) $< | awk -F: '{if (NF > 1) print "obj/"$$0; else print $0}' > $@)


# TARGETS:

all: bin/psi46test
	@true

obj:
	@mkdir -p obj/linux

bin:
	@mkdir -p bin

bin/psi46test: $(addprefix obj/,$(OBJS)) bin rpc_calls.cpp
	$(CXX) -o $@ $(addprefix obj/,$(OBJS)) $(LDFLAGS)
	@echo 'done: bin/psi46test'

o2r: o2r.cc
	g++ $(ROOTCFLAGS) o2r.cc \
	-Wall -O2 -o o2r $(ROOTLIBS)
	@echo 'done: o2r'

x2r: x2r.cc
	g++ $(ROOTCFLAGS) x2r.cc \
	-Wall -O2 -o x2r $(ROOTLIBS)
	@echo 'done: x2r'

tri: tri.cc
	g++ $(ROOTCFLAGS) tri.cc \
	-Wall -O2 -o tri $(ROOTLIBS)
	@echo 'done: tri'

quad: quad.cc
	g++ $(ROOTCFLAGS) quad.cc \
	-Wall -O2 -o quad -L/usr/local/lib64 -lGBL $(ROOTLIBS)
	@echo 'done: quad'

addal4: addal4.cc
	g++ $(ROOTCFLAGS) addal4.cc \
	-Wall -O2 -o addal4
	@echo 'done: addal4'

evd: evd.cc
	g++ $(ROOTCFLAGS) evd.cc \
	-Wall -O2 -o evd $(ROOTGLIBS)
	@echo 'done: evd'

evd4: evd4.cc Makefile
	g++ $(ROOTCFLAGS) evd4.cc \
	-Wall -O2 -o evd4 -L/usr/local/lib64 -lGBL $(ROOTGLIBS)
	@echo 'done: evd4'

evmod: evmod.cc
	g++ $(ROOTCFLAGS) evmod.cc \
	-Wall -O2 -o evmod $(ROOTGLIBS)
	@echo 'done: evmod'

blob: blob.cc
	g++ $(ROOTCFLAGS) blob.cc \
	-Wall -O2 -o blob $(ROOTGLIBS)
	@echo 'done: blob'

ray: ray.cc
	g++ $(ROOTCFLAGS) ray.cc \
	-Wall -O2 -o ray $(ROOTGLIBS)
	@echo 'done: ray'

edge: edge.cc
	g++ $(ROOTCFLAGS) edge.cc \
	-Wall -O2 -o edge $(ROOTLIBS)
	@echo 'done: edge'

modedgerot: modedgerot.cc
	g++ $(ROOTCFLAGS) modedgerot.cc \
	-Wall -O2 -o modedgerot $(ROOTLIBS)
	@echo 'done: modedgerot'

clean:
	rm -rf obj

distclean: clean
	rm -rf bin


# DEPENDENCIES:

#-include $(addprefix obj/,$(OBJS:.o=.d))
