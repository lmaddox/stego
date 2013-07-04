#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <math.h>

#define private static





/** x */
#define M (8)

/** y */
#define N (8)





/** u, x */
private double cosine_x[M][M];

/** v, y */
private double cosine_y[N][N];

/** basis[v][u][y][x] */
private double basis[N][M][N][M];

private double sqrt_1_by_2;
private double sqrt_m_and_n;





private void __attribute__ ((nothrow))
init_cosine_x () {
  uint_fast8_t u, x;
	for (u = 0; u < M; u++)
		for (x = 0; x < M; x++)
			cosine_x[u][x] = cos (M_PI_2 * u / M * (double) (2 * x + 1));
}

private void __attribute__ ((nothrow))
init_cosine_y () {
	uint_fast8_t v, y;
	for (v = 0; v < N; v++)
		for (y = 0; y < N; y++)
			cosine_y[v][y] = cos (M_PI_2 * v / N * (double) (2 * y + 1));
}

/** Write a function to calculate the DCT basis values. I would suggest using
 *  the function to populate a four-dimensional array of floating point values.
 *  E.g., basis[u][v][x][y]. Since there are 4096 values, please, do NOT print
 *  them all to the screen. */
private void __attribute__ ((nothrow))
init_basis () {
	uint_fast8_t v, u, y, x;
	for (v = 0; v < N; v++)
		for (u = 0; u < M; u++)
			for (y = 0; y < N; y++)
				for (x = 0; x < M; x++)
					basis[v][u][y][x] = cosine_x[u][x] * cosine_y[v][y];
}

#ifdef DEBUG
private void __attribute__ ((nothrow))
print_basis () {
	uint_fast8_t v, u, y, x;
	for (v = 0; v < N; v++)
		for (u = 0; u < M; u++) {
			printf ("u=%u, v=%u\n", (unsigned int) u, (unsigned int) v);
			for (y = 0; y < N; y++) {
				for (x = 0; x < M - 1; x++)
					printf ("%10g ", basis[v][u][y][x]);
				printf ("%10g\n", basis[v][u][y][x]);
			}
			puts ("");
		}
}
#endif





private signed char __attribute__ ((const, nothrow, warn_unused_result))
dct_signed (const unsigned char c) {
	return (signed char) c - 128;
}

private void __attribute__ ((nothrow))
kahan (
	double *const sum,
	const double input,
	double *const c
) {
	const double y = input - *c;
	const double t = *sum  + y;
	*c   = (t - *sum) - y;
	*sum = t;
}

private double __attribute__ ((nothrow, pure, warn_unused_result))
alpha (const uint_fast8_t u) {
	if (u == 0)
		return sqrt_1_by_2;
	return 1;
}

private void __attribute__ ((nonnull, nothrow))
dct_transform (double dest[N][M], const unsigned char src[N][M]) {
	uint_fast8_t v, u, y, x;
	memset (dest, 0, N * M * sizeof (double));
	for (v = 0; v < N; v++)
		for (u = 0; u < M; u++) {
			/*dest[v][u] = 0;*/
			double error = 0;
			for (y = 0; y < N; y++)
				for (x = 0; x < M; x++)
					/*kahan (&(dest[v][u]), dct_signed (src[y][x]) * cosine_x[u][x] * cosine_y[v][y], &error);*/
					kahan (&(dest[v][u]), dct_signed (src[y][x]) * basis[v][u][y][x], &error);
			dest[v][u] *= 2.0 / sqrt_m_and_n * alpha (v) * alpha (u);
			dest[v][u] = nearbyint (dest[v][u]);
		}
}

private void __attribute__ ((nonnull, nothrow))
print_orig (const unsigned char orig[N][M]) {
	uint_fast8_t y, x;
	for (y = 0; y < N; y++) {
		for (x = 0; x < M - 1; x++)
			printf ("%3u ", orig[y][x]);
		printf ("%3u\n", orig[y][x]);
	}
	puts ("");
}

private void __attribute__ ((nonnull, nothrow))
print_dct (const double orig[N][M]) {
	uint_fast8_t y, x;
	for (y = 0; y < N; y++) {
		for (x = 0; x < M - 1; x++)
			printf ("%4g ", orig[y][x]);
		printf ("%4g\n", orig[y][x]);
	}
	puts ("");
}

/** Write a function that takes as input a two-dimensional, 8 x 8 array of
 *  unsigned characters, and calculates the DCT. The output should be a two-
 *  -dimensional, 8 x 8 array of floating point values. Since the DCT is a
 *  /signed/ operation, you will first need to convert the unsigned characters
 *  to signed values (i.e., subtract 128 -- yes, it is THAT easy). This function
 *  should print the input matrix to the screen, then print the resulting DCT
 *  matrix. */
private void __attribute__ ((nonnull, nothrow))
dct_transform_bad_design (double dest[N][M], const unsigned char src[N][M]) {
	print_orig (src);
	dct_transform (dest, src);
	print_dct (dest);
}





private unsigned char __attribute__ ((const, nothrow, warn_unused_result))
dct_signed (const signed char c) {
	return (unsigned char) c - 128;
}

private void __attribute__ ((nonnull, nothrow))
idct_transform (unsigned char dest[N][M], const double src[N][M]) {
	uint_fast8_t v, u, y, x;
	for (x = 0; x < M; x++)
		for (y = 0; y < N; y++) {
			double temp  = 0;
			double error = 0;
			signed char temp2;
			for (v = 0; v < N; v++)
				for (u = 0; u < M; u++)
					/*kahan (&temp, alpha (v) * alpha (u) * src[v][u] * cosine_x[u][x] * cosine_y[v][y], &error);*/
					kahan (&temp, alpha (v) * alpha (u) * src[v][u] * basis[v][u][y][x], &error);
			temp *= 2.0 / N;
			temp2 = nearbyint (temp);
			dest[y][x] = dct_signed (temp2);
		}
}

/** Write a function that takes as input a two-dimensional, 8 x 8 array of
 *  floating point numbers and calculates the inverse DCT. The output should be
 *  a two-dimensional, 8 x 8 array of unsigned characters. Since the IDCT is a
 *  /signed/ operation, you will need to convert the signed characters to
 *  unsigned values (i.e., subtract 128 -- yes, it is THAT easy, again). This
 *  function should print the input matrix to the screen, then print the
 *  resulting matrix. If called in succession to the other function, there is no
 *  need to print the floating point matrix twice. */
/*
private void __attribute__ ((nonnull, nothrow))
idct_transform_bad_design (unsigned char dest[N][M], const double src[N][M]) {
	print_dct (src);
	idct_transform (dest, src);
	print_orig (dest);
}
*/
private void __attribute__ ((nonnull, nothrow))
dct_idct_transform (unsigned char dest[N][M], const unsigned char src[N][M]) {
	double temp[N][M];
	/*
	print_orig (src);
	dct_transform (temp, src);
	print_dct (temp);
	*/
	dct_transform_bad_design (temp, src);
	idct_transform (dest, temp);
	print_orig (dest);
}





private int __attribute__ ((nothrow, warn_unused_result))
randint (const int max) {
	int tmp;
	do tmp = rand ();
	while (tmp >= RAND_MAX - RAND_MAX % max);
	return tmp % max;
}

private void __attribute__ ((nonnull, nothrow))
init_all_rand (unsigned char all_rand[N][M]) {
	uint_fast8_t y, x;
	for (y = 0; y < N; y++)
		for (x = 0; x < M; x++)
			all_rand[y][x] = randint (255);
}

int __attribute__ ((nonnull, nothrow, warn_unused_result))
main () {
#ifndef BENCHMARK
	const unsigned char eyebrow[N][M] = {
		{231, 224, 224, 217, 217, 203, 189, 196},
		{210, 217, 203, 189, 203, 224, 217, 224},
		{196, 217, 210, 224, 203, 203, 196, 189},
		{210, 203, 196, 203, 182, 203, 182, 189},
		{203, 224, 203, 217, 196, 175, 154, 140},
		{182, 189, 168, 161, 154, 126, 119, 112},
		{175, 154, 126, 105, 140, 105, 119,  84},
		{154,  98, 105,  98, 105,  63, 112,  84}
	};
	const unsigned char eye[N][M] = {
		{ 42,  28,  35,  28,  42,  49,  35,  42},
		{ 49,  49,  35,  28,  35,  35,  35,  42},
		{ 42,  21,  21,  28,  42,  35,  42,  28},
		{ 21,  35,  35,  42,  42,  28,  28,  14},
		{ 56,  70,  77,  84,  91,  28,  28,  21},
		{ 70, 126, 133, 147, 161,  91,  35,  14},
		{126, 203, 189, 182, 175, 175,  35,  21},
		{ 49, 189, 245, 210, 182,  84,  21,  35}
	};
	const unsigned char nose[N][M] = {
		{154, 154, 175, 182, 189, 168, 217, 175},
		{154, 147, 168, 154, 168, 168, 196, 175},
		{175, 154, 203, 175, 189, 182, 196, 182},
		{175, 168, 168, 168, 140, 175, 168, 203},
		{133, 168, 154, 196, 175, 189, 203, 154},
		{168, 161, 161, 168, 154, 154, 189, 189},
		{147, 161, 175, 182, 189, 175, 217, 175},
		{175, 175, 203, 175, 189, 175, 175, 182}
	};
	const unsigned char all_clear[N][M] = {
		{  0,   0,   0,   0,   0,   0,   0,   0},
		{  0,   0,   0,   0,   0,   0,   0,   0},
		{  0,   0,   0,   0,   0,   0,   0,   0},
		{  0,   0,   0,   0,   0,   0,   0,   0},
		{  0,   0,   0,   0,   0,   0,   0,   0},
		{  0,   0,   0,   0,   0,   0,   0,   0},
		{  0,   0,   0,   0,   0,   0,   0,   0},
		{  0,   0,   0,   0,   0,   0,   0,   0}
	};
	const unsigned char all_set[N][M] = {
		{255, 255, 255, 255, 255, 255, 255, 255},
		{255, 255, 255, 255, 255, 255, 255, 255},
		{255, 255, 255, 255, 255, 255, 255, 255},
		{255, 255, 255, 255, 255, 255, 255, 255},
		{255, 255, 255, 255, 255, 255, 255, 255},
		{255, 255, 255, 255, 255, 255, 255, 255},
		{255, 255, 255, 255, 255, 255, 255, 255},
		{255, 255, 255, 255, 255, 255, 255, 255}
	};
#endif
	unsigned char all_rand[N][M];
/*
	double dct_eyebrow[N][M];
	double dct_eye[N][M];
	double dct_nose[N][M];
*/
#ifndef BENCHMARK
	unsigned char dct_eyebrow[N][M];
	unsigned char dct_eye[N][M];
	unsigned char dct_nose[N][M];
	unsigned char dct_all_clear[N][M];
	unsigned char dct_all_set[N][M];
#endif
	unsigned char dct_all_rand[N][M];

#ifdef BENCHMARK
	int k;
#endif

	srand (getpid () + time (NULL));
	fesetround (FE_TONEAREST);

	sqrt_1_by_2  = sqrt (1.0 / 2.0);
	sqrt_m_and_n = sqrt (M * N);
	init_cosine_x ();
	init_cosine_y ();
	init_basis ();
#ifdef DEBUG
	print_basis ();
#endif
/*
	dct_transform_bad_design (dct_eyebrow, eyebrow);
	dct_transform_bad_design (dct_eye,     eye);
	dct_transform_bad_design (dct_nose,    nose);
*/
#ifndef BENCHMARK
	dct_idct_transform (dct_eyebrow,   eyebrow);
	dct_idct_transform (dct_eye,       eye);
	dct_idct_transform (dct_nose,      nose);
	dct_idct_transform (dct_all_clear, all_clear);
	dct_idct_transform (dct_all_set,   all_set);
#endif

#ifdef BENCHMARK
	for (k = 0; k < 1000; k++) {
#endif
		init_all_rand (all_rand);
		dct_idct_transform (dct_all_rand,  all_rand);
#ifdef BENCHMARK
	}
#endif

	return EXIT_SUCCESS;
}
