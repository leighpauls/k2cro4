/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/barebones/barebones.h"

/*
 * This is a test for the issue described in:
 * http://code.google.com/p/nativeclient/issues/detail?id=2909
 */

void _start(unsigned int info[]) {
#if defined(__arm__)
  char *sp, *scratch;
  __asm__("mov %0, sp" : "=r" (sp));
  __asm__ volatile(
      ".p2align 4\n"
      "bfc %0, #30, #2\n"
      "ldr %0, [%0, #-4095]\n"
      : "=r" (scratch) : "0" (sp + 4095));
#elif defined(__x86_64__)
  unsigned long long scratch;
  __asm__ volatile(
      ".p2align 5\n"
      "leal 4095(%%rsp), %k0\n"
      "movl -4095(%%r15,%0), %k0"
      : "=r" (scratch));
#endif
  NACL_SYSCALL(exit)(0);
}
