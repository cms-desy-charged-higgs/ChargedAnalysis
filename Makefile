.RECIPEPREFIX := $(.RECIPEPREFIX) 

##Compiler/Linking flags
CC      = g++
CFLAGS  = -fPIC -w -std=c++17 -fconcepts -fcompare-debug-second -g -rdynamic -D_GLIBCXX_USE_CXX11_ABI=0 -O2

##Ouput directories
BINDIR=$(CHDIR)/ChargedAnalysis/bin
LIBDIR=$(CHDIR)/ChargedAnalysis/lib
OBJDIR=$(CHDIR)/ChargedAnalysis/obj
 
##Directories with needed header files
INC = -I$(CHDIR)/Anaconda3/lib/python3.8/site-packages/torch/include/torch/csrc/api/include/ -I$(CHDIR)/Anaconda3/lib/python3.8/site-packages/torch/include/torch/ -I$(CHDIR)/Anaconda3/lib/python3.8/site-packages/torch/include -I$(CHDIR)/ -I$(shell root-config --incdir)

##Directories with needed shared libaries
LIBS=-Wl,-rpath,$(shell root-config --libdir) -Wl,-rpath,$(CHDIR)/Anaconda3/lib/python3.8/site-packages/torch/lib -Wl,-rpath,$(LIBDIR) -L$(shell root-config --libdir) -L$(CHDIR)/Anaconda3/lib/python3.8/site-packages/torch/lib -L$(LIBDIR)

##All depedencies of on shared libaries
DEPS=$(shell root-config --noauxlibs --noldflags --glibs) -lpthread -ltorch -lc10 -Wl,--no-as-needed,-ltorch_cpu 

##Target sharged libraries/executables
SOSRC = $(shell ls --color=never $(CHDIR)/ChargedAnalysis/*/src/ | grep .cc)
SOOBJ = $(SOSRC:%.cc=$(OBJDIR)/%.o)
SO=$(CHDIR)/ChargedAnalysis/lib/libChargedAnalysis.so

EXESRC=$(shell ls --color=never $(CHDIR)/ChargedAnalysis/*/exesrc | grep .cc)
EXE=$(EXESRC:%.cc=$(BINDIR)/%)

PYSRC=$(shell ls --color=never $(CHDIR)/ChargedAnalysis/*/exesrc | grep .py)
PYEXE=$(PYSRC:%.py=$(BINDIR)/%.py)

### Target rules ###

all:
    @+make --quiet makedir
    @+make --quiet $(SO)
    @+make --quiet $(PYEXE)
    @+make --quiet $(EXE)

makedir:
    mkdir -p $(OBJDIR)
    mkdir -p $(BINDIR)
    mkdir -p $(LIBDIR)

#### Compile all executables ####

$(BINDIR)/%: $(CHDIR)/ChargedAnalysis/obj/%.o
    echo "Creating executable $@"
    $(CC) $^ -o $@ $(LIBS) $(LIBS) $(DEPS) -lChargedAnalysis

$(BINDIR)/estimate%: $(CHDIR)/ChargedAnalysis/obj/estimate%.o
    echo "Creating executable $@"
    $(CC) $^ -o $@ $(LIBS) $(LIBS) $(DEPS) -lRooFit -lRooFitCore -lChargedAnalysis

$(OBJDIR)/%.o: $(CHDIR)/ChargedAnalysis/*/exesrc/%.cc
    echo "Compiling $<"
    $(CC) $(CFLAGS) -c $^ -o $@ $(CFLAGS) $(INC)

### Copy all python executables to bin dir ###

$(BINDIR)/%.py: $(CHDIR)/ChargedAnalysis/*/exesrc/%.py
    echo "Copy python executable '$<' to bin directory"  
    cp -f $< $(BINDIR)
    chmod a+rx $@

### Compile all objects files origin from classes and the shared library ###

$(SO): $(SOOBJ)
    echo "Building shared library $@"
    $(CC) -Wl,--no-undefined -shared -o $@ $^ $(LIBS) $(DEPS)

$(OBJDIR)/%.o: $(CHDIR)/ChargedAnalysis/*/src/%.cc $(CHDIR)/ChargedAnalysis/*/include/%.h
    echo "Compiling $<"
    $(CC) $(CFLAGS) -c $< -o $@ $(CFLAGS) $(INC)

$(OBJDIR)/%.o: $(CHDIR)/ChargedAnalysis/*/src/%.cc
    echo "Compiling $<"
    $(CC) $(CFLAGS) -c $^ -o $@ $(CFLAGS) $(INC)

### Clean function ###
clean:
    @rm -rf $(OBJDIR)
    @rm -rf $(LIBDIR)
    @rm -rf $(BINDIR)
    @echo "All object files, shared libraries and executables are deleted"
