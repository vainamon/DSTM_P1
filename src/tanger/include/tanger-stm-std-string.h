/* Copyright (C) 2008  Torvald Riegel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/**
 * Wrapper declarations for string.h
 */
#ifndef TANGERSTMSTDSTRING_H_
#define TANGERSTMSTDSTRING_H_

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void * memset(void *src, int c, size_t n) __attribute__ ((tm_wrapper("tanger_stm_std_memset")));
void * memcpy(void *dest, const void *src, size_t n) __attribute__ ((tm_wrapper("tanger_stm_std_memcpy")));
void * memmove(void *dest, const void *src, size_t n) __attribute__ ((tm_wrapper("tanger_stm_std_memmove")));

/* intrinsics */
static void tanger_wrapper_tanger_stm_intrinsic_memset_i64(void* dest, uint8_t val, uint64_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memset.p0i8.i64")));
static void tanger_wrapper_tanger_stm_intrinsic_memset_i32(void* dest, uint8_t val, uint32_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memset.p0i8.i32")));
#if !defined(TANGER_LLVM_VERSION) || (TANGER_LLVM_VERSION > 204)
static void tanger_wrapper_tanger_stm_intrinsic_memset_i16(void* dest, uint8_t val, uint16_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memset.p0i8.i16")));
static void tanger_wrapper_tanger_stm_intrinsic_memset_i8(void* dest, uint8_t val, uint8_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memset.p0i8.i8")));
#endif
static void tanger_wrapper_tanger_stm_intrinsic_memcpy_i64(void* dest, void* src, uint64_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memcpy.p0i8.p0i8.i64")));
static void tanger_wrapper_tanger_stm_intrinsic_memcpy_i32(void* dest, void* src, uint32_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memcpy.p0i8.p0i8.i32")));
#if !defined(TANGER_LLVM_VERSION) || (TANGER_LLVM_VERSION > 204)
static void tanger_wrapper_tanger_stm_intrinsic_memcpy_i16(void* dest, void* src, uint16_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memcpy.p0i8.p0i8.i16")));
static void tanger_wrapper_tanger_stm_intrinsic_memcpy_i8(void* dest, void* src, uint8_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memcpy.p0i8.p0i8.i8")));
#endif
static void tanger_wrapper_tanger_stm_intrinsic_memmove_i64(void* dest, void* src, uint64_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memmove.p0i8.p0i8.i64")));
static void tanger_wrapper_tanger_stm_intrinsic_memmove_i32(void* dest, void* src, uint32_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memmove.p0i8.p0i8.i32")));
#if !defined(TANGER_LLVM_VERSION) || (TANGER_LLVM_VERSION > 204)
static void tanger_wrapper_tanger_stm_intrinsic_memmove_i16(void* dest, void* src, uint16_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memmove.p0i8.p0i8.i16")));
static void tanger_wrapper_tanger_stm_intrinsic_memmove_i8(void* dest, void* src, uint8_t len, uint32_t align, uint8_t volatil)
    __attribute__ ((weakref("tanger.llvm.memmove.p0i8.p0i8.i8")));
#endif

#ifdef __cplusplus
}
#endif

#endif /*TANGERSTMSTDSTRING_H_*/
