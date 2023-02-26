// fft.h
// Berechnet die Fouriertransformierte einer Funktion
// kopiert von https://rosettacode.org/wiki/Fast_Fourier_transform
// Created and modified by Matthias Kesenheimer, m.kesenheimer@gmx.net
#pragma once
#include "constants.h"
#include <complex>
#include <valarray>

namespace math::utilities
{
	/// <summary>
	/// 
	/// </summary>
	class FFT
	{
	public:
		/// <summary>
		///
		/// </summary>
		template<class Vect>
		static void fft(Vect& real, Vect& imag)
		{
			typedef typename Vect::value_type type;
			typedef typename Vect::size_type size_type;

			if (real.size() != imag.size()) return;

			std::valarray<std::complex<type>> cmplx(std::complex<type>(0, 0), real.size());
			for (size_type i = 0; i < real.size(); ++i)
				cmplx[i] = std::complex<type>(real[i], imag[i]);

			fft<type, size_type>(cmplx);

			for (size_type i = 0; i < real.size(); ++i)
			{
				real[i] = cmplx[i].real();
				imag[i] = cmplx[i].imag();
			}
		}

		/// <summary>
		///
		/// </summary>
		template<class Vect>
		static void ifft(Vect& real, Vect& imag)
		{
			typedef typename Vect::value_type type;
			typedef typename Vect::size_type size_type;

			if (real.size() != imag.size()) return;

			std::valarray<std::complex<type>> cmplx(std::complex<type>(0, 0), real.size());
			for (size_type i = 0; i < real.size(); ++i)
				cmplx[i] = std::complex<type>(real[i], imag[i]);

			ifft<type, size_type>(cmplx);

			for (size_type i = 0; i < real.size(); ++i)
			{
				real[i] = cmplx[i].real();
				imag[i] = cmplx[i].imag();
			}
		}

		/// <summary>
		///
		/// </summary>
		template<class type, class size_type>
		static void fft(std::valarray<std::complex<type>>& x)
		{
			fft1<type, size_type>(x);
		}

		/// <summary>
		///
		/// </summary>
		template<class type, class size_type>
		static void ifft(std::valarray<std::complex<type>>& x)
		{
			ifft2<type, size_type>(x);
		}

	private:
		// Cooley-Tukey FFT (in-place, divide-and-conquer)
		// Higher memory requirements and redundancy although more intuitive
		/// <summary>
		///
		/// </summary>
		template<class type, class size_type>
		static void fft1(std::valarray<std::complex<type>>& x)
		{
			const size_type N = x.size();
			if (N <= 1) return;

			// divide
			std::valarray<std::complex<type>> even = x[std::slice(0, N / 2, 2)];
			std::valarray<std::complex<type>> odd = x[std::slice(1, N / 2, 2)];

			// conquer
			fft1<type, size_type>(even);
			fft1<type, size_type>(odd);

			// combine
			for (size_type k = 0; k < N / 2; ++k)
			{
				std::complex<type> t = std::polar(1.0, -2 * constants::Pi::value() * k / N) * odd[k];
				x[k] = even[k] + t;
				x[k + N / 2] = even[k] - t;
			}
		}

		// Cooley-Tukey FFT (in-place, breadth-first, decimation-in-frequency)
		// Better optimized but less intuitive 
		/// <summary>
		///
		/// </summary>
		template<class type, class size_type>
		static void fft2(std::valarray<std::complex<type>>& x)
		{
			// DFT
			size_type N = x.size(), k = N;
			type thetaT = static_cast<type>(constants::Pi::value() / N);
			std::complex<type> phiT = std::complex<type>(cos(thetaT), -sin(thetaT)), T;
			while (k > 1)
			{
				size_type n = k;
				k >>= 1;
				phiT = phiT * phiT;
				T = 1.0;
				for (size_type l = 0; l < k; l++)
				{
					for (size_type a = l; a < N; a += n)
					{
						size_type b = a + k;
						if (a < x.size() && b < x.size())
						{
							std::complex<type> t = x[a] - x[b];
							x[a] += x[b];
							x[b] = t * T;
						}
					}
					T *= phiT;
				}
			}
			// Decimate
			size_type m = static_cast<size_type>(log2(N));
			for (size_type a = 0; a < N; a++)
			{
				size_type b = a;
				// Reverse bits
				b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
				b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
				b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
				b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
				b = ((b >> 16) | (b << 16)) >> (32 - m);
				if (b > a)
				{
					std::complex<type> t = x[a];
					x[a] = x[b];
					x[b] = t;
				}
			}

			//// Normalize (This section make it not working correctly)
			//std::complex<type> f = 1.0 / sqrt(N);
			//for (size_type i = 0; i < N; i++)
			//	x[i] *= f;
		}

		// inverse fft (in-place)
		/// <summary>
		///
		/// </summary>
		template<class type, class size_type>
		static void ifft2(std::valarray<std::complex<type>>& x)
		{
			// conjugate the complex numbers
			x = x.apply(std::conj);

			// forward fft
			fft2<type, size_type>(x);

			// conjugate the complex numbers again
			x = x.apply(std::conj);

			// scale the numbers
			x /= static_cast<type>(x.size());
		}
	};
}