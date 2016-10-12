#pragma once

/*
* Reference: DSD2PCM, https://code.google.com/archive/p/dsd2pcm/; available under BSD license
*/
/*

Copyright 2009, 2011 Sebastian Gesemann. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

THIS SOFTWARE IS PROVIDED BY SEBASTIAN GESEMANN ''AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEBASTIAN GESEMANN OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Sebastian Gesemann.

----

Additions (c) Adrian Smith, 2013 under same licence terms:
- expose bitreverse array as dsd2pcm_bitreverse
- expose precalc function as dsd2pcm_precalc to allow it to be initalised

*/

class Noiseshaper
{
private:
	int sos_count;      /* number of second order sections */
	std::vector<float> t1, t2; /* filter state, owned by ns library */

	const float my_ns_coeffs[8] = {
		//     b1           b2           a1           a2
		-1.62666423f,  0.79410094f,  0.61367127f,  0.23311013f,  // section 1
		-1.44870017f,  0.54196219f,  0.03373857f,  0.70316556f   // section 2
	};
public:
	Noiseshaper() : sos_count(sizeof(my_ns_coeffs) / sizeof(my_ns_coeffs[0]) * 4), t1(sos_count, 0.0f), t2(sos_count, 0.0f)
	{
	}
	Noiseshaper(const Noiseshaper& other)
	{
		sos_count = other.sos_count;
		t1.assign(other.t1.begin(), other.t1.end());
		t2.assign(other.t2.begin(), other.t2.end());
	}

	Noiseshaper& operator=(const Noiseshaper& other)
	{
		if (this == &other)
			return *this;
		t1.assign(other.t1.begin(), other.t1.end());
		t2.assign(other.t2.begin(), other.t2.end());
	}

	~Noiseshaper()
	{
	}

	void reset()
	{
		for (int i = 0; i < sos_count; ++i) {
			t1[i] = t2[i] = 0.0f;
		}
	}

	float get()
	{
		float acc = 0.0;
		const float* c = my_ns_coeffs;
		for (int i = 0; i < sos_count; ++i) {
			float t1i = t1[i];
			float t2i = t2[i];
			t2[i] = (acc -= t1i * c[2] + t2i * c[3]);
			acc += t1i * c[0] + t2i * c[1];
			c += 4;
		}
		return acc;
	}

	void update(float qerror)
	{
		for (int i = 0; i < sos_count; ++i) {
			t2[i] += qerror;
		}
		std::swap(t1, t2);
	}
};