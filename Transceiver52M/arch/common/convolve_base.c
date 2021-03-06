/*
 * Convolution
 * Copyright (C) 2012, 2013 Thomas Tsou <tom@tsou.cc>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <malloc.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Base multiply and accumulate complex-real */
static void mac_real(const float *x, const float *h, float *y)
{
	y[0] += x[0] * h[0];
	y[1] += x[1] * h[0];
}

/* Base multiply and accumulate complex-complex */
static void mac_cmplx(const float *x, const float *h, float *y)
{
	y[0] += x[0] * h[0] - x[1] * h[1];
	y[1] += x[0] * h[1] + x[1] * h[0];
}

/* Base vector complex-complex multiply and accumulate */
static void mac_real_vec_n(const float *x, const float *h, float *y,
			   int len, int step, int offset)
{
	for (int i = offset; i < len; i += step)
		mac_real(&x[2 * i], &h[2 * i], y);
}

/* Base vector complex-complex multiply and accumulate */
static void mac_cmplx_vec_n(const float *x, const float *h, float *y,
			    int len, int step, int offset)
{
	for (int i = offset; i < len; i += step)
		mac_cmplx(&x[2 * i], &h[2 * i], y);
}

/* Base complex-real convolution */
int _base_convolve_real(const float *x, int x_len,
			const float *h, int h_len,
			float *y, int y_len,
			int start, int len,
			int step, int offset)
{
	for (int i = 0; i < len; i++) {
		mac_real_vec_n(&x[2 * (i - (h_len - 1) + start)],
			       h,
			       &y[2 * i], h_len,
			       step, offset);
	}

	return len;
}

/* Base complex-complex convolution */
int _base_convolve_complex(const float *x, int x_len,
			   const float *h, int h_len,
			   float *y, int y_len,
			   int start, int len,
			   int step, int offset)
{
	for (int i = 0; i < len; i++) {
		mac_cmplx_vec_n(&x[2 * (i - (h_len - 1) + start)],
				h,
				&y[2 * i],
				h_len, step, offset);
	}

	return len;
}

/* Buffer validity checks */
int bounds_check(int x_len, int h_len, int y_len,
		 int start, int len, int step)
{
	if ((x_len < 1) || (h_len < 1) ||
	    (y_len < 1) || (len < 1) || (step < 1)) {
		fprintf(stderr, "Convolve: Invalid input\n");
		return -1;
	}

	if ((start + len > x_len) || (len > y_len) || (x_len < h_len)) {
		fprintf(stderr, "Convolve: Boundary exception\n");
		fprintf(stderr, "start: %i, len: %i, x: %i, h: %i, y: %i\n",
				start, len, x_len, h_len, y_len);
		return -1;
	}

	return 0;
}

/* API: Non-aligned (no SSE) complex-real */
int base_convolve_real(const float *x, int x_len,
		       const float *h, int h_len,
		       float *y, int y_len,
		       int start, int len,
		       int step, int offset)
{
	if (bounds_check(x_len, h_len, y_len, start, len, step) < 0)
		return -1;

	memset(y, 0, len * 2 * sizeof(float));

	return _base_convolve_real(x, x_len,
				   h, h_len,
				   y, y_len,
				   start, len, step, offset);
}

/* API: Non-aligned (no SSE) complex-complex */
int base_convolve_complex(const float *x, int x_len,
			  const float *h, int h_len,
			  float *y, int y_len,
			  int start, int len,
			  int step, int offset)
{
	if (bounds_check(x_len, h_len, y_len, start, len, step) < 0)
		return -1;

	memset(y, 0, len * 2 * sizeof(float));

	return _base_convolve_complex(x, x_len,
				      h, h_len,
				      y, y_len,
				      start, len, step, offset);
}

/* Aligned filter tap allocation */
void *convolve_h_alloc(int len)
{
#ifdef HAVE_SSE3
	return memalign(16, len * 2 * sizeof(float));
#else
	return malloc(len * 2 * sizeof(float));
#endif
}
