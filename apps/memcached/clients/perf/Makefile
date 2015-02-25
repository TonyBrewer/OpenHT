SRCS = $(wildcard *.cpp)

perf_client: $(SRCS)
	$(CXX) -I/hw/tools/systemc/ht_lib/host \
	-g -m64 -Wall -o $@ -lpthread $(SRCS)

clean:
	rm -f perf_client
