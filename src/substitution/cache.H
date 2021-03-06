/*
  Copyright (C) 2004-2005,2007,2010 Benjamin Redelings

  This file is part of BAli-Phy.

  BAli-Phy is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2, or (at your option) any later
  version.

  BAli-Phy is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License
  along with BAli-Phy; see the file COPYING.  If not see
  <http://www.gnu.org/licenses/>.  */

#ifndef SUBSTITUTION_CACHE_H
#define SUBSTITUTION_CACHE_H

#include "object.H"
#include <boost/shared_ptr.hpp>
#include <vector>

constexpr double scale_factor = 115792089237316195423570985008687907853269984665640564039457584007913129639936e0;
constexpr double scale_min = 1.0/scale_factor;
constexpr double log_scale_min = -177.445678223345999210811423093293201427328034396225345054e0;

/// An object to store cached conditional likelihoods for a single branch
class Likelihood_Cache_Branch: public Object
{
    double* data = nullptr;
    int* scale_ = nullptr;
    int matrix_size_ = -1;

    int n_states_ = -1;
    int n_models_ = -1;
    int n_columns_ = -1;
public:
    Likelihood_Cache_Branch* clone() const {std::abort();}
  
    log_double_t other_subst = 1;

    int n_states() const {return n_states_;}
    int n_models() const {return n_models_;}
    int n_columns() const {return n_columns_;}
    int matrix_size() const {return matrix_size_;}
  
    double* scratch(int i) {assert(0 <= i and i<2); return data + matrix_size()*(n_columns() + i);}

    double* operator[](int i) {assert(0 <= i and i < n_columns()); return data + matrix_size()*i;}
    const double* operator[](int i) const {assert(0 <= i and i < n_columns()); return data + matrix_size()*i;}

    int& scale(int i)       {assert(0 <= i and i < n_columns()); return scale_[i];}
    int  scale(int i) const {assert(0 <= i and i < n_columns()); return scale_[i];}

    void set(int c, double d)
    {
	double* m = data + matrix_size()*c;
	for(int i=0;i<matrix_size();i++)
	    m[i] = d;
    }

    void set(int c, const Matrix& M)
    {
	double* __restrict__ m1 = data + matrix_size()*c;
	const double* __restrict__ m2 = M.begin();
	for(int i=0;i<matrix_size();i++)
	    m1[i] = m2[i];
    }

    void mul(int c, const Matrix& M)
    {
	double* __restrict__ m1 = data + matrix_size()*c;
	const double* __restrict__ m2 = M.begin();
	for(int i=0;i<matrix_size();i++)
	    m1[i] *= m2[i];
    }

    void mul(int c, double f)
    {
	double* m1 = data + matrix_size()*c;
	for(int i=0;i<matrix_size();i++)
	    m1[i] *= f;
    }

    double dot(int c, const Matrix& M)
    {
	const double* __restrict__ m1 = data + matrix_size()*c;
	const double* __restrict__ m2 = M.begin();
	double total = 0;
	for(int i=0;i<matrix_size();i++)
	    total += m1[i] * m2[i];
	return total;
    }

    double sum(int c)
    {
	const double* m1 = data + matrix_size()*c;
	double total = 0;
	for(int i=0;i<matrix_size();i++)
	    total += m1[i];
	return total;
    }

    void swap(Likelihood_Cache_Branch& LCB)
    {
	std::swap(data, LCB.data);
	std::swap(scale_, LCB.scale_);
	std::swap(matrix_size_, LCB.matrix_size_);
	std::swap(n_states_, LCB.n_states_);
	std::swap(n_models_, LCB.n_models_);
	std::swap(n_columns_, LCB.n_columns_);
    }

    Likelihood_Cache_Branch& operator=(const Likelihood_Cache_Branch&) = delete;
    Likelihood_Cache_Branch& operator=(Likelihood_Cache_Branch&& LCB) noexcept
    {
	swap(LCB);
	return *this;
    }

    Likelihood_Cache_Branch() {}
    Likelihood_Cache_Branch(const Likelihood_Cache_Branch&) = delete;
    Likelihood_Cache_Branch(int C,int M, int S)
	:data(new double[(C+2)*M*S]),
	 scale_(new int[C]),
	 matrix_size_(M*S),
	 n_states_(S),
	 n_models_(M),
	 n_columns_(C)
	{
	    for(int i=0;i<C;i++)
		scale_[i] = 0;
	}
    Likelihood_Cache_Branch(Likelihood_Cache_Branch&& LCB) noexcept
    {
	swap(LCB);
    }

    ~Likelihood_Cache_Branch() {delete[] data; delete[] scale_;}
};

#endif
