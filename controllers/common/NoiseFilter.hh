/*
	Noise filtering based on FFT.
	Chopping down the high-frequency noise results in stable readings.
 */

#ifndef AEGIR_NOISEFILTER_H
#define AEGIR_NOISEFILTER_H

#include <complex.h>
#include <fftw3.h>

namespace aegir {

	// S: size, total number of samples
	// BL: BucketLength, number of samples to aggregate over
	template<int S, int BL=5>
	class NoiseFilter {
	public:
		static constexpr int samples = S/BL;
		struct datapoint {
			bool present;
			double value;
		};
		struct input {
			datapoint points[S];
		};
	public:
		NoiseFilter()=delete;

		NoiseFilter(double _chop=0.001f): chop(_chop) {
			transformed = fftw_alloc_complex(samples);
			plan_f = fftw_plan_dft_r2c_1d(samples, in, transformed,
																		FFTW_ESTIMATE);
			if ( plan_f == 0 )
				throw Exception("FFTW forward plan failed");

			plan_i = fftw_plan_dft_c2r_1d(samples, transformed, out,
																		FFTW_ESTIMATE);
			if ( plan_i == 0 ) {
				fftw_destroy_plan(plan_f);
				throw Exception("FFTW inverse plan failed");
			}
		};

		~NoiseFilter() {
			fftw_destroy_plan(plan_f);
			fftw_destroy_plan(plan_i);
			fftw_free((void*)transformed);
		};

		double aggregate(const input &_data) {
			float insum=0.0f;
			int incnt=0;
			for (int i=0; i<S; ++i) {
				if ( _data.points[i].present ) {
					insum += _data.points[i].value;
					++incnt;
				}
			}
			insum /= 1.0f*incnt;
			readInput(_data);

			// do the forward fft
			fftw_execute(plan_f);

			// chop samples
			for (int i=0; i<samples/2; ++i) {
				if ( fabs(transformed[i][0]) < chop ) {
					transformed[i][0] = 0.0;
					transformed[i][1] = 0.0;
				}
			}

			// and do the inverse fft
			fftw_execute(plan_i);

			// average out the results
			double sum(0.0);
			int skipped(0);
			for (int i=0; i<samples; ++i) {
				//double x = out[i]/((samples-1)*2);
				double x = out[i]/(samples);
				if ( x < 0.5f ) {
					++skipped;
					continue;
				}
				sum += x;
				//printf("out[%i]: %.4f avg:%0.4f\n", i, x, sum/(i+1-skipped));
			}
			//printf("inavg: %.4f\n", insum);
			return sum/(1.0f*(samples-skipped));
		}

	private:
		void readInput(const input &_data) {
			int s;
			double v;
			for (int i=0; i<samples; ++i) {
				s = 0;
				v = 0.0f;
				for (int j=i*BL; j<i*(BL)+BL; ++j) {
					if (_data.points[j].present) {
						++s;
						v += _data.points[j].value;
					}
				}
				in[i] = s>0?(v/s):0.0f;
			}
		}

	private:
		double in[samples], out[samples], chop;
		fftw_complex *transformed;
		fftw_plan plan_f, plan_i;
	};

}

#endif
