// Copyright 2017 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_PORT_PORT_CONFIG_H_
#define STORAGE_LEVELDB_PORT_PORT_CONFIG_H_

// Define to 1 if you have a definition for fdatasync() in <unistd.h>.
#if !defined(HAVE_FDATASYNC)
#define HAVE_FDATASYNC 0
#endif  // !defined(HAVE_FDATASYNC)

// Define to 1 if you have a definition for F_FULLFSYNC in <fcntl.h>.
#if !defined(HAVE_FULLFSYNC)
#define HAVE_FULLFSYNC 0
#endif  // !defined(HAVE_FULLFSYNC)

// Define to 1 if you have a definition for O_CLOEXEC in <fcntl.h>.
#if !defined(HAVE_O_CLOEXEC)
#define HAVE_O_CLOEXEC 0
#endif  // !defined(HAVE_O_CLOEXEC)

// Define to 1 if you have Google CRC32C.
#if !defined(HAVE_CRC32C)
#define HAVE_CRC32C 0
#endif  // !defined(HAVE_CRC32C)

// Define to 1 if you have Google Snappy.
#if !defined(HAVE_SNAPPY)
#define HAVE_SNAPPY 0
#endif  // !defined(HAVE_SNAPPY)

// Define to 1 if you have Zstd.
#if !defined(HAVE_Zstd)
#define HAVE_ZSTD 0
#endif  // !defined(HAVE_ZSTD)

#endif  // STORAGE_LEVELDB_PORT_PORT_CONFIG_H_
