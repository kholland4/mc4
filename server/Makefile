CXXFLAGS = -std=c++17 -pthread -Wall -Wextra -O3
LIBS = -lsqlite3
SRCS = $(wildcard *.cpp)
OBJDIR = build
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

mc4-server: $(OBJS)
	$(CXX) $(LIBS) -pthread -o $@ $(OBJS)

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MD -c -o $@ $<

-include $(SRCS:%.cpp=%.d)

clean:
	rm $(OBJS) $(OBJS:%.o=%.d) mc4-server
