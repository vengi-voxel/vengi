# Setup commands

- compile: `make`
- run particular tests: `GTEST_FILTER=SomeTest.* make tests-yourmodulename-run`
- run all tests: `make tests`

# Code style

- try to avoid the STL where possible!

# General

- when touching code, ensure that a test is written for it - if not, write one to test the change - and execute that test

# Logging

- when adding debug logging via `Log::debug()`, make sure to use the environment variable `CORE_LOGLEVEL=2` in front of your vengi applications or your `make` calls.
