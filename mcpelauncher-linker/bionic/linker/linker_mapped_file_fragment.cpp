/*
 * Copyright (C) 2015 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "linker_mapped_file_fragment.h"
#include "linker_debug.h"
#include "linker_utils.h"

#include <inttypes.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__aarch64__)
#define page_start __page_start
#define page_offset __page_offset
static off64_t __page_start(off64_t offset) {
  return offset & ~static_cast<off64_t>(4*4096-1);
}

static size_t __page_offset(off64_t offset) {
  return static_cast<size_t>(offset & (4*4096-1));
}
#elif defined(__linux__)
#include <unistd.h>
#ifdef _SC_PAGESIZE
#define page_start __page_start
#define page_offset __page_offset
static off64_t __page_start(off64_t offset) {
  return offset & ~static_cast<off64_t>(sysconf(_SC_PAGESIZE)-1);
}

static size_t __page_offset(off64_t offset) {
  return static_cast<size_t>(offset & (sysconf(_SC_PAGESIZE)-1));
}
#endif
#endif

MappedFileFragment::MappedFileFragment() : map_start_(nullptr), map_size_(0),
                                           data_(nullptr), size_ (0)
{ }

MappedFileFragment::~MappedFileFragment() {
  if (map_start_ != nullptr) {
    munmap(map_start_, map_size_);
  }
}

bool MappedFileFragment::Map(int fd, off64_t base_offset, size_t elf_offset, size_t size) {
  off64_t offset;
  CHECK(safe_add(&offset, base_offset, elf_offset));

  off64_t page_min = page_start(offset);
  off64_t end_offset;

  CHECK(safe_add(&end_offset, offset, size));
  CHECK(safe_add(&end_offset, end_offset, page_offset(offset)));

  size_t map_size = static_cast<size_t>(end_offset - page_min);
  CHECK(map_size >= size);

  uint8_t* map_start = static_cast<uint8_t*>(
                          mmap64(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, page_min));

  if (map_start == MAP_FAILED) {
    return false;
  }

  map_start_ = map_start;
  map_size_ = map_size;

  data_ = map_start + page_offset(offset);
  size_ = size;

  return true;
}
