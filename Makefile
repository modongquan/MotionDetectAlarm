CC = gcc
CXX = g++

TARGET = mdalarm

INCDIR := -I./inc \
	-I./inc/spdlog \
	-I./inc/mysql \
	-I./src

SRCDIR := ./src
SRC_OBJS := $(patsubst %.cpp,%.o,$(wildcard $(SRCDIR)/*.cpp))
SRC_OBJS := $(notdir $(SRC_OBJS))

LIBS = -lmysqlclient \
       -lpthread \
       -lhpr \
       -lHCCore \
       -lhcnetsdk \

LIBPATH = ./lib

OBJSDIR := ./build

LDFLAG = -Wl,-rpath=$(LIBPATH)

CXXFLAGS = -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC

$(TARGET) : $(SRC_OBJS)

	$(CXX) $^ -o $@ -L$(LIBPATH) $(LIBS) $(LDFLAG)
	mv $^ $(OBJSDIR)

$(SRC_OBJS) : $(wildcard $(SRCDIR)/*.cpp)

	$(CXX) -c $^ $(INCDIR) $(CXXFLAGS)

clean:

	rm -f $(OBJSDIR)/*.o
	rm -f $(TARGET)

.PHONY:clean
