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

eclipse:
	$(Q)./fips config linux-eclipse-debug

server: build
	$(Q)./fips run server

client: build
	$(Q)./fips run client

debugserver: build
	$(Q)./fips gdb server

debugclient: build
	$(Q)./fips gdb client

tests: build
	$(Q)./fips run tests

tests-list: build
	$(Q)./fips run tests -- --gtest_list_tests

tests-filter: build
	$(Q)./fips run tests -- --gtest_filter=$(FILTER)
