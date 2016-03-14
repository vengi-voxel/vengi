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

server:
	$(Q)./fips run server

client:
	$(Q)./fips run client

generate:
	$(Q)./fips run worldgenerator -- -set seed 1 -set size 64
