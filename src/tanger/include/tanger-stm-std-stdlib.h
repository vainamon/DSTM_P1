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
 * Wrapper declarations for stdlib.h
 */
#ifndef TANGERSTMSTDSTDLIB_H_
#define TANGERSTMSTDSTDLIB_H_

#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

void qsort(void *base, size_t nel, size_t width,
        int (*compar)(const void *, const void *))
__attribute__ ((tm_wrapper("tanger_stm_std_qsort")));

#ifdef __cplusplus
}
#endif

#endif /*TANGERSTMSTDSTDLIB_H_*/
