// -*- c++ -*-

// 1. implement "fixed" generator
// 2. implement discrete generator
// 3. implement combine generator? 

#ifndef GENERATOR_H
#define GENERATOR_H

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#include "config.h"

#include <string>
#include <vector>
#include <utility>
#include <random>
#include <algorithm>
#include <iostream>

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "util.h"

// Generator syntax:
//
// \d+ == fixed
// n[ormal]:mean,sd
// e[xponential]:lambda
// p[areto]:scale,shape
// g[ev]:loc,scale,shape
// fb_value, fb_key, fb_rate

class Generator {
	public:
		Generator() {}
		//  Generator(const Generator &g) = delete;
		//  virtual Generator& operator=(const Generator &g) = delete;
		virtual ~Generator() {}

		virtual double generate(double U = -1.0) = 0;
		virtual void set_lambda(double lambda) {DIE("set_lambda() not implemented");}
	protected:
		std::string type;
};

class Fixed : public Generator {
	public:
		Fixed(double _value = 1.0) : value(_value) { D("Fixed(%f)", value); }
		virtual double generate(double U = -1.0) { return value; }
		virtual void set_lambda(double lambda) {
			if (lambda > 0.0) value = 1.0 / lambda;
			else value = 0.0;
		}

	private:
		double value;
};

struct ZipfianCmp {
	double asDouble( const std::pair<int,double>& v ) const // or static
	{
		return v.second;
	}

	double asDouble( double t ) const // or static
	{
		return t;
	}

	template< typename T1, typename T2 >
		bool operator()( T1 const& t1, T2 const& t2 ) const
		{
			return asDouble(t1) < asDouble(t2);
		}
};

class Zipfian : public Generator {
	public:
		Zipfian(double _alpha, uint64_t _range) : alpha(_alpha), range(_range) {
			D("Zipfian(%f,%lu)", alpha,range); 
			cdf.reserve(1000000);
			scale = 0;

			//std::string dist_fname = "zipfian-"+std::to_string(alpha)+"-"+std::to_string(range)+".dist";
			std::string dist_fname = "zipf-"+std::to_string(alpha)+".dist";
			FILE * fin = fopen(dist_fname.c_str(),"r");
			uint64_t j=0;
			double c;
			if(fin!=NULL){
				std::cerr << "Using the existing distribution file: " << dist_fname << "\n";
				while(j<range && fscanf(fin,"%lu %lf",&j,&c)==2){
					cdf.push_back(std::pair<uint64_t,double>(j,c));
				}
				scale = c;
				fclose(fin);
				std::cerr << "Finished reading in the distribution\n";
			}

			std::cerr << "Generating and stroing the rest of zipfian: range: " << range << "\n";
			double partial = 0;

			std::vector<std::pair<uint64_t,double>> cdf_rest;


#pragma omp for ordered schedule(dynamic)
			for(uint64_t i=j+1; i<=range; ++i){
				double pdf = 1.0/ pow((double)i,alpha); 

#pragma omp ordered
				{
					partial+=pdf;

					if((partial > 0.000001) || (i==range) ){
						scale+=partial;
						//std::cerr << cdf.size() << "\t" << scale << "\t" << partial << "\n";
						cdf_rest.push_back(std::pair<uint64_t,double>(i,scale));
						partial = 0;
					}
				}
			}
			
			FILE * fout = fopen(dist_fname.c_str(),"a");
			for(auto v: cdf_rest){
				fprintf(fout,"%lu %lf\n",v.first,v.second);
				cdf.push_back(v);
			}
			fclose(fout);
			std::cerr << "Finished writing the distribution data\n";
			std::cerr << "scale is: " << scale << "range is: " << range << "\n";
		}

		virtual double generate(double U= -1.0) {
			if(U < 0.0) U=drand48();
			U*=scale;
			//double D = dis(gen);
			//std::cerr << "generated double: " << U << "\n";
			std::vector<std::pair<uint64_t,double>>::iterator it, next_it;
			it = std::lower_bound(cdf.begin(), cdf.end(), std::pair<uint64_t,double>(0,U), ZipfianCmp()); 
			next_it = it;
			next_it++;

			//std::cerr << "it->first: " << it->first << "\tnext_it->first: " << next_it->first << "\n";

			uint64_t index = lrand48()%(next_it->first - it->first)+it->first;
			//std::cerr << "index is: " << index << "\t value is: " << U << "\n";
			return index;
		}

	private:
		double scale, alpha;
		uint64_t range;
		std::vector<std::pair<uint64_t,double>> cdf;
};

class Uniform : public Generator {
	public:
		Uniform(double _scale) : scale(_scale) { D("Uniform(%f)", scale); }

		virtual double generate(double U = -1.0) {
			if (U < 0.0) U = drand48();
			return scale * U;
		}

		virtual void set_lambda(double lambda) {
			if (lambda > 0.0) scale = 2.0 / lambda;
			else scale = 0.0;
		}

	private:
		double scale;
};

class Normal : public Generator {
	public:
		Normal(double _mean = 1.0, double _sd = 1.0) : mean(_mean), sd(_sd) {
			D("Normal(mean=%f, sd=%f)", mean, sd);
		}

		virtual double generate(double U = -1.0) {
			double rv;
			do {
				if (U < 0.0) U = drand48();
				double V = U; // drand48();
				double N = sqrt(-2 * log(U)) * cos(2 * M_PI * V);
				rv = mean + sd * N;
			} while(rv < 0);
			return rv;
		}

		virtual void set_lambda(double lambda) {
			if (lambda > 0.0) mean = 1.0 / lambda;
			else mean = 0.0;
		}

	private:
		double mean, sd;
};

class Exponential : public Generator {
	public:
		Exponential(double _lambda = 1.0) : lambda(_lambda) {
			D("Exponential(lambda=%f)", lambda);
		}

		virtual double generate(double U = -1.0) {
			if (lambda <= 0.0) return 0.0;
			if (U < 0.0) U = drand48();
			return -log(U) / lambda;
		}

		virtual void set_lambda(double lambda) { this->lambda = lambda; }

	private:
		double lambda;
};

class GPareto : public Generator {
	public:
		GPareto(double _loc = 0.0, double _scale = 1.0, double _shape = 1.0) :
			loc(_loc), scale(_scale), shape(_shape) {
				assert(shape != 0.0);
				D("GPareto(loc=%f, scale=%f, shape=%f)", loc, scale, shape);
			}

		virtual double generate(double U = -1.0) {
			if (U < 0.0) U = drand48();
			double res = loc + scale * (pow(U, -shape) - 1) / shape;
			//std::cerr << "U: " << U << "\t " << res << "\n";
			return res;
		}

		virtual void set_lambda(double lambda) {
			if (lambda <= 0.0) scale = 0.0;
			else scale = (1 - shape) / lambda - (1 - shape) * loc;
		}

	private:
		double loc /* mu */;
		double scale /* sigma */, shape /* k */;
};

class GEV : public Generator {
	public:
		GEV(double _loc = 0.0, double _scale = 1.0, double _shape = 1.0) :
			e(1.0), loc(_loc), scale(_scale), shape(_shape) {
				assert(shape != 0.0);
				D("GEV(loc=%f, scale=%f, shape=%f)", loc, scale, shape);
			}

		virtual double generate(double U = -1.0) {
			double res = loc + scale * (pow(e.generate(U), -shape) - 1) / shape;
			//std::cerr << "U: " << U << "\t" << res << "\n";
			return res;
		}

	private:
		Exponential e;
		double loc /* mu */, scale /* sigma */, shape /* k */;
};

class Discrete : public Generator {
	public:
		~Discrete() { delete def; }
		Discrete(Generator* _def = NULL) : def(_def) {
			if (def == NULL) def = new Fixed(0.0);
		}

		virtual double generate(double U = -1.0) {
			double Uc = U;
			if (pv.size() > 0 && U < 0.0) U = drand48();

			double sum = 0;

			for (auto p: pv) {
				sum += p.first;
				if (U < sum) return p.second;
			}

			return def->generate(Uc);
		}

		void add(double p, double v) {
			pv.push_back(std::pair<double,double>(p, v));
		}

	private:
		Generator *def;
		std::vector< std::pair<double,double> > pv;
};

class KeyGenerator {
	public:
		KeyGenerator(Generator* _g, double _max = 10000) : g(_g), max(_max) { std::cerr << "_max: " << _max << "\n";}

		std::string generate(uint64_t ind) {
			uint64_t h = fnv_64(ind);
			double U = (double) h*0.999 / ULLONG_MAX;
			double G = g->generate(U);
			int keylen = MAX(round(G), floor(log10(max)) + 1);
			char key[256];
			snprintf(key, 256, "%0*" PRIu64, keylen, ind);

			//    D("%d = %s", ind, key);
			//std::cerr << "length of key: " << keylen << "\tround(G): " << round(G) << "\tlog10(max): " << floor(log10(max)) << "\t" << max << "\t" << std::string(key) << "\t" << G << "\n";
			return std::string(key);
		}

	private:
		Generator* g;
		double max;
};

Generator* createGenerator(std::string str);
Generator* createFacebookKey();
Generator* createFacebookValue();
Generator* createFacebookIA();

#endif // GENERATOR_H
