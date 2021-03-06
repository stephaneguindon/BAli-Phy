/*
  Copyright (C) 2004-2007,2010 Benjamin Redelings

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

/**
 * @file setup.H
 *
 * @brief This file contains routines for parsing the command line and
 *        loading information from files in order to start the MCMC
 *        analysis.
 */

#ifndef SETUP_H
#define SETUP_H

#include <vector>
#include <string>
#include <valarray>
#include "alignment/alignment.H"
#include "tree/sequencetree.H"
#include <boost/program_options.hpp>

//----------------- Load trees and alignments ---------------------//

/// Map leaf nodes of T to the leaf sequences of A
void link(alignment& A,SequenceTree& T,bool internal_sequences=true);

SequenceTree load_constraint_tree(const std::string& filename,const std::vector<std::string>&);


/// Count how many times each letter occurs in A
std::valarray<double> letter_counts(const alignment& A);

/// Count how many times letter 'l' occurss in A (including *,-,?)
int letter_count(const alignment& A,int l);

/// Load statements from a config file.
void load_bali_phy_rc(boost::program_options::variables_map&,
		      const boost::program_options::options_description&);

//--------------------------- Indel stuff --------------------------//

template <typename T>
class shared_items
{
    // unique items
    std::vector<T> items;

public:

    // from partition -> item
    std::vector<int> item_for_partition;  

    // from item -> partition
    std::vector<std::vector<int> > partitions_for_item;

    int n_unique_items() const {return items.size();}

    int n_partitions() const {return item_for_partition.size();}

    const T& unique(int i) const {return items[i];}
    T& unique(int i)       {return items[i];}

    const T& operator[](int i) const {return items[item_for_partition[i]];}
    T& operator[](int i)       {return items[item_for_partition[i]];}

    int n_partitions_for_item(int i) const {return partitions_for_item[i].size();}

    shared_items(const std::vector<T>& v1, const std::vector<int>& v2)
	:items(v1),
	 item_for_partition(v2),
	 partitions_for_item(items.size())
	{
	    for(int i=0;i<n_partitions();i++) {
		int item = item_for_partition[i];
		if (item != -1)
		    partitions_for_item[item].push_back(i);
	    }
	}
};

std::string parse_partitions_and_model(std::string model, std::vector<int>& partitions);
shared_items<std::string> get_mapping(const boost::program_options::variables_map& args, const std::string& key, int n);


#endif 
