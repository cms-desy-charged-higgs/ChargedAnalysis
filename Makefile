.RECIPEPREFIX+=

##Compiler/Linking flags
CC      = $(CHDIR)/Anaconda3/bin/g++
CFLAGS  = -I$(CHDIR) -fPIC -g
LDFLAGS = -L$(CHDIR)/ChargedAnalysis/Analysis/lib -L$(CHDIR)/Anaconda3/lib

PYFLAGS_C = ${shell $(CHDIR)/Anaconda3/bin/python3.7-config --cflags}
PYFLAGS_LD = ${shell $(CHDIR)/Anaconda3/bin/python3.7-config --ldflags}
ROOTFLAGS_C = $(shell root-config --cflags)
ROOTFLAGS_LD = $(shell root-config --ldflags --glibs) -lTMVA -lGenVector

##Source and target directories
OBJDIR = $(CHDIR)/ChargedAnalysis/Analysis/obj
SRCDIR = $(CHDIR)/ChargedAnalysis/Analysis/src
HDIR = $(CHDIR)/ChargedAnalysis/Analysis/include
LIBDIR = $(CHDIR)/ChargedAnalysis/Analysis/lib
BINDIR = $(CHDIR)/ChargedAnalysis/Analysis/bin

##Target executbales
TARGETS = $(BINDIR)/Plot1D $(BINDIR)/TreeRead

##Compile plotter class in shared libary and compile plot1d exetuable
PLOTSRC = $(shell ls $(SRCDIR) --color=never | grep plot)
PLOTOBJ = $(PLOTSRC:%.cc=$(OBJDIR)/%.o)

all : $(TARGETS)

$(BINDIR)/Plot1D: $(OBJDIR)/plot1d.o $(LIBDIR)/libPlot.so
    @echo "Create binary $@"
    @$(CC) $(LDFLAGS) $(ROOTFLAGS_LD) -lPlot -o $@ $^

$(OBJDIR)/plot1d.o: $(BINDIR)/plot1d.cc $(HDIR)/plotter.h
    @mkdir -p $(OBJDIR)

    @echo "Compiling file $<"
    @$(CC) $(CFLAGS) $(ROOTFLAGS_C) -o $@ -c $<

$(LIBDIR)/libPlot.so: $(PLOTOBJ)
    @mkdir -p $(LIBDIR)

    @echo "Build shared libary $@"
    @$(CC) -shared -o $@ $^

$(OBJDIR)/plot%.o: $(SRCDIR)/plot%.cc $(HDIR)/plot%.h
    @mkdir -p $(OBJDIR)

    @echo "Compiling file $<" 
    @$(CC) $(CFLAGS) $(ROOTFLAGS_C) -o $@ -c $<

##Compile treeread executable
TREESRC = treereader.cc treereaderfunction.cc
TREEOBJ = $(TREESRC:%.cc=$(OBJDIR)/%.o)

MLSRC = dnn.cc bdt.cc
MLOBJ = $(MLSRC:%.cc=$(OBJDIR)/%.o)

$(BINDIR)/TreeRead: $(OBJDIR)/treeread.o $(TREEOBJ) $(LIBDIR)/libML.so
    @echo "Create binary $@"
    @$(CC) $(LDFLAGS) $(ROOTFLAGS_LD) $(PYFLAGS_LD) -lML -o $@ $^

$(OBJDIR)/treeread.o: $(BINDIR)/treeread.cc $(HDIR)/treereader.h
    @mkdir -p $(OBJDIR)

    @echo "Compiling file $<"
    @$(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYFLAGS_C) -o $@ -c $<

$(OBJDIR)/tree%.o: $(SRCDIR)/tree%.cc $(HDIR)/treereader.h $(HDIR)/dnn.h $(HDIR)/bdt.h
    @mkdir -p $(OBJDIR)

    @echo "Compiling file $<" 
    @$(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYFLAGS_C) -o $@ -c $<

##Compile machine learning related classes
$(LIBDIR)/libML.so: $(MLOBJ)
    @mkdir -p $(LIBDIR)    

    @echo "Build shared libary $@"
    @$(CC) -shared -o $@ $^

$(OBJDIR)/bdt.o: $(SRCDIR)/bdt.cc $(HDIR)/bdt.h
    @mkdir -p $(OBJDIR)

    @echo "Compiling file $<" 
    @$(CC) $(CFLAGS) $(ROOTFLAGS_C) -o $@ -c $<

$(OBJDIR)/dnn.o: $(SRCDIR)/dnn.cc $(HDIR)/dnn.h
    @mkdir -p $(OBJDIR)

    @echo "Compiling file $<" 
    @$(CC) $(CFLAGS) $(ROOTFLAGS_C) $(PYFLAGS_C) -o $@ -c $<


##Clean function
clean:
    @rm -rf $(OBJDIR)
    @echo "All object files in ChargedAnalysis/Analysis/obj are cleaned up"

    @rm -rf $(LIBDIR)
    @echo "All shared libaries in ChargedAnalysis/Analysis/lib are cleaned up"

    @rm -rf $(TARGETS)
    @echo "All executables in ChargedAnalysis/Analysis/bin are cleaned up"
