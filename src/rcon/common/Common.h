#pragma once

#define COMPILE_TIME_ASSERT(statement) extern int COMPILE_TIME_ASSERT_ARRAY[((statement) != 0) * 2 - 1]
#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(*(array)))
