CXX := c++
CXXFLAGS := -std=c++11 -Wall -Wextra -g -fsanitize=address,undefined

ifeq ($(shell uname -s),Darwin)
SDK_CXX_INCLUDE := -isystem $(shell xcrun --show-sdk-path)/usr/include/c++/v1
endif

OUTPUTS := test_dsp test_plugin

all: $(OUTPUTS)

test_dsp: test_dsp.cpp ../dsp.h
	$(CXX) $(CXXFLAGS) -o $@ $< -lm

test_plugin: test_plugin.cpp ../vortex_stereo.cpp ../dsp.h
	$(CXX) $(CXXFLAGS) $(SDK_CXX_INCLUDE) -I../distingNT_API/include -o $@ $< -lm

run: $(OUTPUTS)
	./test_dsp
	./test_plugin

clean:
	rm -f $(OUTPUTS)
	rm -rf test_dsp.dSYM test_plugin.dSYM

.PHONY: all run clean
