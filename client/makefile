CXX = g++

CXXFLAGS = -Wall -g

LDFLAGS = -lssl -lcrypto

TARGET = client

SRCS = client.cpp upload_download.cpp basic_commands.cpp file_divider.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
		$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
		./$(TARGET)

clean:
		rm -rf $(TARGET) $(OBJS)