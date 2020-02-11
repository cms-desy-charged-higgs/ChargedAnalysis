.RECIPEPREFIX+=

##Compiler/Linking flags
CC      = g++
CFLAGS  = -fPIC -w -std=c++17 -g

##Directories with needed header files
INC = -I$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/include/torch/csrc/api/include/ -I$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/include/torch/ -I$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/include -I$(CHDIR)/ -L$(CHDIR)/ChargedAnalysis/Utility/lib -I$(shell root-config --incdir)

##Directories with needed shared libaries
LIBS=-L$(shell root-config --libdir) -L$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/lib -L$(CHDIR)/ChargedAnalysis/Utility/lib -L$(CHDIR)/ChargedAnalysis/Analysis/lib -L$(CHDIR)/ChargedAnalysis/Network/lib

##All depedencies of on shared libaries
DEPS=$(shell root-config --noauxlibs --noldflags --glibs) -lGX11 -lTMVA -lGenVector -lpthread -ltorch -lc10 -lcaffe2_detectron_ops_gpu -lUtil -lTrees -lPlot -lML

##Target sharged libraries/executables
SO=$(CHDIR)/ChargedAnalysis/Analysis/lib/libTrees.so $(CHDIR)/ChargedAnalysis/Analysis/lib/libPlot.so  $(CHDIR)/ChargedAnalysis/Utility/lib/libUtil.so $(CHDIR)/ChargedAnalysis/Network/lib/libML.so

EXESRC=$(shell ls --color=never $(CHDIR)/ChargedAnalysis/*/bin/* | grep cc)
EXE=$(EXESRC:%.cc=%)

all:
    @+make --quiet makedir
    @+make --quiet $(SO)
    @+make --quiet $(EXE)

makedir:
    for DIR in Analysis Network Utility; do \
        mkdir -p $(CHDIR)/ChargedAnalysis/$$DIR/obj; \
        mkdir -p $(CHDIR)/ChargedAnalysis/$$DIR/lib; \
    done    

#### Compile all executables ####
#### Compile all executables ####
$(CHDIR)/ChargedAnalysis/Analysis/bin/%: $(CHDIR)/ChargedAnalysis/Analysis/obj/%.o
    echo "Creating executable $@"
    g++ $^ -o $@ $(LIBS) $(DEPS)

$(CHDIR)/ChargedAnalysis/Utility/bin/%: $(CHDIR)/ChargedAnalysis/Utility/obj/%.o
    echo "Creating executable $@"
    g++ $^ -o $@ $(LIBS) $(DEPS)

$(CHDIR)/ChargedAnalysis/Network/bin/%: $(CHDIR)/ChargedAnalysis/Network/obj/%.o
    echo "Creating executable $@"
    g++ $^ -o $@ $(LIBS) $(DEPS)
#### Build shared libaries
TREESRC = $(shell ls --color=never $(CHDIR)/ChargedAnalysis/Analysis/src | grep tree)
TREEOBJ = $(TREESRC:%.cc=$(CHDIR)/ChargedAnalysis/Analysis/obj/%.o)

$(CHDIR)/ChargedAnalysis/Analysis/lib/libTrees.so: $(TREEOBJ)
    echo "Building shared library $@"
    $(CC) -o $@ $^ -shared

UTILSRC = $(shell ls --color=never $(CHDIR)/ChargedAnalysis/Utility/src)
UTILOBJ = $(UTILSRC:%.cc=$(CHDIR)/ChargedAnalysis/Utility/obj/%.o)

$(CHDIR)/ChargedAnalysis/Utility/lib/libUtil.so: $(UTILOBJ)
    echo "Building shared library $@"
    $(CC) -o $@ $^ -shared

PLOTSRC = $(shell ls --color=never $(CHDIR)/ChargedAnalysis/Analysis/src | grep plot)
PLOTOBJ = $(PLOTSRC:%.cc=$(CHDIR)/ChargedAnalysis/Analysis/obj/%.o)

$(CHDIR)/ChargedAnalysis/Analysis/lib/libPlot.so: $(PLOTOBJ)
    echo "Building shared library $@"
    $(CC) -o $@ $^ -shared

MLSRC = $(shell ls --color=never $(CHDIR)/ChargedAnalysis/Network/src)
MLOBJ = $(MLSRC:%.cc=$(CHDIR)/ChargedAnalysis/Network/obj/%.o)

$(CHDIR)/ChargedAnalysis/Network/lib/libML.so: $(MLOBJ)
    echo "Building shared library $@"
    $(CC) -o $@ $^ -shared

#### Compile all classes ####
$(CHDIR)/ChargedAnalysis/Utility/obj/%.o: $(CHDIR)/ChargedAnalysis/Utility/*/%.cc
    echo "Compiling $<"
    $(CC) -c $^ -o $@ $(CFLAGS) $(INC)

$(CHDIR)/ChargedAnalysis/Analysis/obj/%.o: $(CHDIR)/ChargedAnalysis/Analysis/*/%.cc
    echo "Compiling $<"
    $(CC) -c $^ -o $@ $(CFLAGS) $(INC)

$(CHDIR)/ChargedAnalysis/Network/obj/%.o: $(CHDIR)/ChargedAnalysis/Network/*/%.cc
    echo "Compiling $<"
    $(CC) -c $^ -o $@ $(CFLAGS) $(INC)

##Clean function
clean:
    @rm -rf $(CHDIR)/ChargedAnalysis/*/obj
    @rm -rf $(CHDIR)/ChargedAnalysis/*/lib
    @rm -rf $(EXE)
    @echo "All object files, shared libraries and executables are deleted"
