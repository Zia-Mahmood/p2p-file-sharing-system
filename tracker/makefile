CXX = g++

CXXFLAGS = -Wall -g

TARGET = tracker

SRCS = tracker.cpp data_serialization.cpp group.cpp response_generators.cpp tracker_exit.cpp user.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
		$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
		./$(TARGET)

clean:
		rm -rf $(TARGET) $(OBJS)