#include <math.h>
#include "fourier.h"

#define SWAP(a, b)  tempr=(a); (a)=(b); (b)=tempr

/*
 * Einfache Fast-Fourier Transformation gemäss Numerical Recipes, second edition.
 */
void four1(float* data, unsigned long nn, int isign)
{
	unsigned long n, mmax, m, j, istep, i;
	double wtemp, wr, wpr, wpi, wi, theta, thetabase;
	float tempr, tempi;

	// Reorder by inverse binary
	n = nn << 1;
	j = 1;
	for (i = 1; i < n; i +=2) {
		if (j > i) {
			SWAP(data[j], data[i]);
			SWAP(data[j+1], data[i+1]);
		}
		m = n >> 1;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1;
		}
		j += m;
	}

	// Danielson-Lanczos
	mmax = 2;
	thetabase = isign * 2. * M_PI;
	while (n > mmax) {
		istep = mmax << 1;
		theta = thetabase / mmax;
		wtemp = sin(0.5 * theta);
		wpr = -2. * wtemp * wtemp;
		wpi = sin(theta);
		wr = 1.;
		wi = 0.;
		for (m = 1; m < mmax; m += 2) {
			for (i = m; i <= n; i += istep) {
				j = i + mmax;
				tempr = wr * data[j] - wi * data[j + 1];
				tempi = wr * data[j + 1] + wi * data[j];
				data[j] = data[i] - tempr;
				data[j + 1] = data[i + 1] - tempi;
				data[i] += tempr;
				data[i + 1] += tempi;
			}
			wtemp = wr;
			wr = wr * wpr - wi * wpi + wr;
			wi = wi * wpr + wtemp * wpi + wi;
		}
		mmax = istep;
	}
}

/*
 * Fourier-Transformation von reellen Zahlen.
 */
void realft(float* data, unsigned long n, int isign)
{
	unsigned long i, i1, i2, i3, i4, np3;
	float c1 = 0.5, c2, h1r, h1i, h2r, h2i;
	double wr, wi, wpr, wpi, wtemp, theta;

	theta = M_PI / (n >> 1);
	if (isign == 1) {
		c2 = -0.5;
		four1(data, n >> 1, isign);
	} else {
		c2 = 0.5;
		theta = - theta;
	}
	wtemp = sin(0.5 * theta);
	wpr = -2. * wtemp * wtemp;
	wpi = sin(theta);
	wr = 1.0 + wpr;
	wi = wpi;
	np3 = n + 3;
	for (i = 2; i <= (n >> 2); i++) {
		i4 = 1 + (i3 = np3 - (i2 = 1 + (i1 = i + i - 1)));
		h1r = c1 * (data[i1] + data[i3]);
		h1i = c1 * (data[i2] - data[i4]);
		h2r = -c2 * (data[i2] + data[i4]);
		h2i = c2 * (data[i1] - data[i3]);
		data[i1] = h1r + wr * h2r - wi * h2i;
		data[i2] = h1i + wr * h2i + wi * h2r;
		data[i3] = h1r - wr * h2r + wi * h2i;
		data[i4] = -h1i + wr * h2i + wi * h2r;
		wr = (wtemp = wr) * wpr - wi * wpi + wr;
		wi = wi * wpr + wtemp * wpi + wi;
	}
	if (isign == 1) {
		data[1] = (h1r = data[1]) + data[2];
		data[2] = (h1r - data[2]);
	} else {
		data[1] = c1 * ((h1r = data[1]) + data[2]);
		data[2] = c1 * (h1r - data[2]);
		four1(data, n >>1, isign);
	}
}
