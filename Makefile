NT_API_PATH := distingNT_API
INCLUDE_PATH := $(NT_API_PATH)/include
SRC := vortex_stereo.cpp
OUTPUT := plugins/vortex_stereo.o
MANIFEST := plugins/plugin.json
VERSION := $(shell cat VERSION)
ARM_CXX ?= arm-none-eabi-c++

CXXFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
	-mthumb -fno-rtti -fno-exceptions -O3 -fno-math-errno \
	-fno-trapping-math -fPIC -Wall -Wextra \
	-I$(INCLUDE_PATH) -DVORTEX_VERSION='"$(VERSION)"'

all: $(OUTPUT) $(MANIFEST)

$(OUTPUT): $(SRC) dsp.h VERSION
	mkdir -p plugins
	$(ARM_CXX) $(CXXFLAGS) -c -o $@ $<

$(MANIFEST): VERSION
	mkdir -p plugins
	@echo '{"name":"Vortex_stereo","guid":"VtxS","version":"$(VERSION)","author":"wintocode; stereo adaptation for Giovanni Lami","description":"Unofficial stereo adaptation of Vortex by wintocode, v$(VERSION)","tags":["effect","filter"]}' > $@

test:
	$(MAKE) -C tests run

package: all
	mkdir -p dist/Vortex_stereo-$(VERSION)
	cp $(OUTPUT) $(MANIFEST) README.md RELEASE_NOTES.md ORIGIN_AND_CHANGES.md LICENSE THIRD_PARTY_NOTICES.md dist/Vortex_stereo-$(VERSION)/
	cd dist && zip -r -FS Vortex_stereo-$(VERSION).zip Vortex_stereo-$(VERSION)

clean:
	rm -f $(OUTPUT) $(MANIFEST)
	$(MAKE) -C tests clean

.PHONY: all test package clean
