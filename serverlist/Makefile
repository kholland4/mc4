CXXFLAGS = -std=c++17 -pthread -Wall -Wextra -O3
LIBS = -lcurl
SRCS = $(wildcard *.cpp)
OBJDIR = build
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

mc4-serverlist: $(OBJS)
	$(CXX) $(LDFLAGS) -pthread -o $@ $(OBJS) $(LIBS)

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MD -c -o $@ $<

-include $(SRCS:%.cpp=%.d)

clean:
	rm $(OBJS) $(OBJS:%.o=%.d) mc4-serverlist
