.RECIPEPREFIX+=

##Compiler/Linking flags
CC      = $(CHDIR)/Anaconda3/bin/g++
CFLAGS  = -I$(CHDIR) -fPIC -g
LDFLAGS = -L$(CHDIR)/Anaconda3/lib -L$(CHDIR)/ChargedAnalysis/Analysis/lib -L$(CHDIR)/ChargedAnalysis/Network/lib

ROOTFLAGS_C = $(shell root-config --cflags)
ROOTFLAGS_LD = $(shell root-config --ldflags --glibs) -lGX11 -lTMVA -lGenVector

PYTORCH_C = -I$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/include/torch/csrc/api/include/ -I$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/include/torch/ -I$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/include
PYTORCH_LD = -L$(CHDIR)/Anaconda3/lib/python3.7/site-packages/torch/lib -ltorch -lc10 -lcaffe2_detectron_ops_gpu -D_GLIBCXX_USE_CXX11_ABI=0

##Source and target directories for ChargedAnalysis
A_OBJDIR = $(CHDIR)/ChargedAnalysis/Analysis/obj
A_SRCDIR = $(CHDIR)/ChargedAnalysis/Analysis/src
A_HDIR = $(CHDIR)/ChargedAnalysis/Analysis/include
A_LIBDIR = $(CHDIR)/ChargedAnalysis/Analysis/lib
A_BINDIR = $(CHDIR)/ChargedAnalysis/Analysis/bin

##Source and target directories for ChargedNetwork
N_OBJDIR = $(CHDIR)/ChargedAnalysis/Network/obj
N_SRCDIR = $(CHDIR)/ChargedAnalysis/Network/src
N_HDIR = $(CHDIR)/ChargedAnalysis/Network/include
N_LIBDIR = $(CHDIR)/ChargedAnalysis/Network/lib
N_BINDIR = $(CHDIR)/ChargedAnalysis/Network/bin

##Target executbales
BINARIES = $(A_BINDIR)/Plot $(A_BINDIR)/TreeRead $(A_BINDIR)/TreeAppend $(A_BINDIR)/FileSkim $(A_BINDIR)/WriteCard $(A_BINDIR)/Limit $(A_BINDIR)/PlotLimit $(N_BINDIR)/HTag
LIBARIES = $(A_LIBDIR)/libPlot.so $(A_LIBDIR)/libUtils.so $(N_LIBDIR)/libML.so $(A_LIBDIR)/libTrees.so

all:
    @+make --quiet $(LIBARIES)
    @+make --quiet $(BINARIES)

########################### Executable for plotter ###########################

$(A_BINDIR)/Plot: $(A_OBJDIR)/plot.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lPlot -lUtils -o $@ $^

$(A_OBJDIR)/plot.o: $(A_BINDIR)/plot.cc 
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Executable for treeread ###########################

$(A_BINDIR)/TreeRead: $(A_OBJDIR)/treeread.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lML -lUtils -lTrees -o $@ $^

$(A_OBJDIR)/treeread.o: $(A_BINDIR)/treeread.cc
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Executable for treeappend ###########################

$(A_BINDIR)/TreeAppend: $(A_OBJDIR)/treeappend.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lML -lUtils -lTrees -o $@ $^

$(A_OBJDIR)/treeappend.o: $(A_BINDIR)/treeappend.cc
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Executable for fileskim ###########################

$(A_BINDIR)/FileSkim: $(A_OBJDIR)/fileskim.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lUtils -o $@ $^

$(A_OBJDIR)/fileskim.o: $(A_BINDIR)/fileskim.cc
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Executable for datacard ###########################

$(A_BINDIR)/WriteCard: $(A_OBJDIR)/writecard.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lUtils -o $@ $^

$(A_OBJDIR)/writecard.o: $(A_BINDIR)/writecard.cc
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Executable for limit ###########################

$(A_BINDIR)/Limit: $(A_OBJDIR)/limit.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lUtils -o $@ $^

$(A_OBJDIR)/limit.o: $(A_BINDIR)/limit.cc
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Executable for plot limit ###########################

$(A_BINDIR)/PlotLimit: $(A_OBJDIR)/plotlimit.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lUtils -lPlot -o $@ $^

$(A_OBJDIR)/plotlimit.o: $(A_BINDIR)/plotlimit.cc
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Executable for htagger training ###########################

$(N_BINDIR)/HTag: $(N_OBJDIR)/htag.o
    echo "Create binary $@"
    $(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYTORCH_LD) -lUtils -lML -lTrees -lPlot -o $@ $^

$(N_OBJDIR)/htag.o: $(N_BINDIR)/htag.cc
    mkdir -p $(N_OBJDIR)

    echo "Compiling file $<"
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

########################### Shared libaries ###########################

TREESRC = treereader.cc treereaderfunction.cc treeappender.cc
TREEOBJ = $(TREESRC:%.cc=$(A_OBJDIR)/%.o)

#Tree related libaries
$(A_LIBDIR)/libTrees.so: $(TREEOBJ)
    mkdir -p $(A_LIBDIR)

    echo "Build shared libary $@"
    $(CC) $(LDFLAGS) -shared -o $@ $^

$(A_OBJDIR)/treeread%.o: $(A_SRCDIR)/treeread%.cc $(A_HDIR)/treereader.h $(MLH)
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<" 
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

$(A_OBJDIR)/treeappend%.o: $(A_SRCDIR)/treeappend%.cc $(A_HDIR)/treeappender.h
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<" 
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

#Plot libary
PLOTSRC = $(shell ls $(A_SRCDIR) --color=never | grep plot)
PLOTOBJ = $(PLOTSRC:%.cc=$(A_OBJDIR)/%.o)

$(A_LIBDIR)/libPlot.so: $(PLOTOBJ)
    mkdir -p $(A_LIBDIR)

    echo "Build shared libary $@"
    $(CC) -shared -o $@ $^

$(A_OBJDIR)/plot%.o: $(A_SRCDIR)/plot%.cc $(A_HDIR)/plot%.h
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<" 
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) -o $@ -c $<

##Machine learning libary
MLSRC = bdt.cc htagger.cc
MLOBJ = $(MLSRC:%.cc=$(N_OBJDIR)/%.o)
MLH = $(MLSRC:%.cc=$(N_HDIR)/%.h)

$(N_LIBDIR)/libML.so: $(MLOBJ)
    mkdir -p $(N_LIBDIR)

    echo "Build shared libary $@"
    $(CC) -shared -o $@ $^

$(N_OBJDIR)/%.o: $(N_SRCDIR)/%.cc $(N_HDIR)/%.h
    mkdir -p $(N_OBJDIR)

    echo "Compiling file $<" 
    $(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYTORCH_C) -o $@ -c $<

##Utility libary
UTILSRC = utils.cc frame.cc datacard.cc
UTILOBJ = $(UTILSRC:%.cc=$(A_OBJDIR)/%.o)
UTILH = $(UTILSRC:%.cc=$(A_HDIR)/%.h)

$(A_LIBDIR)/libUtils.so: $(UTILOBJ)
    mkdir -p $(A_LIBDIR)    

    echo "Build shared libary $@"
    $(CC) -shared -o $@ $^

$(A_OBJDIR)/%.o: $(A_SRCDIR)/%.cc $(A_HDIR)/%.h
    mkdir -p $(A_OBJDIR)

    echo "Compiling file $<" 
    $(CC) $(CFLAGS) $(PYTORCH_C) $(ROOTFLAGS_C) -o $@ -c $<


##Clean function
clean:
    @rm -rf $(A_OBJDIR)
    @echo "All object files in ChargedAnalysis/Analysis/obj are cleaned up"
    @rm -rf $(N_OBJDIR)
    @echo "All object files in ChargedAnalysis/Network/obj are cleaned up"

    @rm -rf $(A_LIBDIR)
    @echo "All shared libaries in ChargedAnalysis/Analysis/lib are cleaned up"
    @rm -rf $(N_LIBDIR)
    @echo "All shared libaries in ChargedAnalysis/Network/lib are cleaned up"

    @rm -rf $(TARGETS)
    @echo "All executables in ChargedAnalysis/Analysis/bin are cleaned up"
    @echo "All executables in ChargedAnalysis/Network/bin are cleaned up"
