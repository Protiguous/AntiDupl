/*
* Simd Library.
*
* Copyright (c) 2011-2013 Yermalayeu Ihar.
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
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "Simd/SimdEnable.h"
#include "Simd/SimdMemory.h"
#include "Simd/SimdLoad.h"
#include "Simd/SimdStore.h"
#include "Simd/SimdConst.h"
#include "Simd/SimdDeinterleaveUv.h"

namespace Simd
{
	namespace Base
	{
		void DeinterleaveUv(const uchar * uv, size_t uvStride, size_t width, size_t height, 
			uchar * u, size_t uStride, uchar * v, size_t vStride)
		{
			for(size_t row = 0; row < height; ++row)
			{
				for(size_t col = 0, offset = 0; col < width; ++col, offset += 2)
				{
					u[col] = uv[offset];
					v[col] = uv[offset + 1];
				}
				uv += uvStride;
				u += uStride;
				v += vStride;
			}
		}
	}

#ifdef SIMD_SSE2_ENABLE    
	namespace Sse2
	{
		SIMD_INLINE __m128i DeinterleavedU(__m128i uv0, __m128i uv1)
		{
			return _mm_packus_epi16(_mm_and_si128(uv0, K16_00FF), _mm_and_si128(uv1, K16_00FF));
		}

		SIMD_INLINE __m128i DeinterleavedV(__m128i uv0, __m128i uv1)
		{
			return DeinterleavedU(_mm_srli_si128(uv0, 1), _mm_srli_si128(uv1, 1));
		}

		template <bool align> void DeinterleaveUv(const uchar * uv, size_t uvStride, size_t width, size_t height, 
			uchar * u, size_t uStride, uchar * v, size_t vStride)
		{
			assert(width >= A);
			if(align)
			{
				assert(Aligned(uv) && Aligned(uvStride) && Aligned(u) && Aligned(uStride) && Aligned(v) && Aligned(vStride));
			}

			size_t bodyWidth = AlignLo(width, A);
			size_t tail = width - bodyWidth;
			for(size_t row = 0; row < height; ++row)
			{
				for(size_t col = 0, offset = 0; col < bodyWidth; col += A, offset += DA)
				{
					__m128i uv0 = Load<align>((__m128i*)(uv + offset));
					__m128i uv1 = Load<align>((__m128i*)(uv + offset + A));
					Store<align>((__m128i*)(u + col), DeinterleavedU(uv0, uv1));
					Store<align>((__m128i*)(v + col), DeinterleavedV(uv0, uv1));
				}
				if(tail)
				{
					size_t col = width - A;
					size_t offset = 2*col;
					__m128i uv0 = Load<false>((__m128i*)(uv + offset));
					__m128i uv1 = Load<false>((__m128i*)(uv + offset + A));
					Store<false>((__m128i*)(u + col), DeinterleavedU(uv0, uv1));
					Store<false>((__m128i*)(v + col), DeinterleavedV(uv0, uv1));
				}
				uv += uvStride;
				u += uStride;
				v += vStride;
			}
		}

		void DeinterleaveUv(const uchar * uv, size_t uvStride, size_t width, size_t height, 
			uchar * u, size_t uStride, uchar * v, size_t vStride)
		{
			if(Aligned(uv) && Aligned(uvStride) && Aligned(u) && Aligned(uStride) && Aligned(v) && Aligned(vStride))
				DeinterleaveUv<true>(uv, uvStride, width, height, u, uStride, v, vStride);
			else
				DeinterleaveUv<false>(uv, uvStride, width, height, u, uStride, v, vStride);
		}
	}
#endif// SIMD_SSE2_ENABLE

	void DeinterleaveUv(const uchar * uv, size_t uvStride, size_t width, size_t height, 
		uchar * u, size_t uStride, uchar * v, size_t vStride)
	{
#ifdef SIMD_SSE2_ENABLE
		if(Sse2::Enable && width >= Sse2::A)
			Sse2::DeinterleaveUv(uv, uvStride, width, height, u, uStride, v, vStride);
		else
#endif//SIMD_SSE2_ENABLE
			Base::DeinterleaveUv(uv, uvStride, width, height, u, uStride, v, vStride);
	}

	void DeinterleaveUv(const View & uv, View & u, View & v)
	{
		assert(uv.width == u.width && uv.height == u.height && uv.width == v.width && uv.height == v.height);
		assert(uv.format == View::Uv16 && u.format == View::Gray8 && v.format == View::Gray8);

		DeinterleaveUv(uv.data, uv.stride, uv.width, uv.height, u.data, u.stride, v.data, v.stride);
	}
}