/* Copyright © 2013, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef DLWRAP_H
#define DLWRAP_H

#include <dlfcn.h>

/* Call the *real* dlopen. We have our own wrapper for dlopen that, of
 * necessity must use claim the symbol 'dlopen'. So whenever anything
 * internal needs to call the real, underlying dlopen function, the
 * thing to call is dlwrap_real_dlopen.
 */
void *
dlwrap_real_dlopen (const char *filename, int flag);

/* Call the *real* dlsym. We have our own wrapper for dlsym that, of
 * necessity must use claim the symbol 'dlsym'. So whenever anything
 * internal needs to call the real, underlying dlysm function, the
 * thing to call is dlwrap_real_dlsym.
 */
void *
dlwrap_real_dlsym (void *handle, const char *symbol);

#endif

