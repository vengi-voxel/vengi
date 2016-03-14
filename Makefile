TARGET=
VERBOSE=
Q=@

all: build

build:
	$(Q)./fips make VERBOSE=$(VERBOSE)

clean:
	$(Q)./fips clean
	$(Q)rm -f .fips-gen.py
	$(Q)rm -f .fips-imports.cmake

run: build
	$(Q)./fips run $(TARGET)

eclipse:
	$(Q)./fips config linux-eclipse-debug

server: build
	$(Q)./fips run server -- -set core_loglevel 4

client: build
	$(Q)./fips run client -- -set core_loglevel 2

generate: build
	$(Q)./fips run worldgenerator -- -set seed 1 -set size 64
