/*
  Copyright (C) 2004-2009 Benjamin Redelings

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

#include <fstream>
#include "tree-dist.H"
#include "io.H"
#include "read-trees.H"

using std::vector;
using std::valarray;

using std::string;
using std::endl;
using std::cerr;
using std::cout;
using std::istream;

using boost::dynamic_bitset;
using boost::shared_ptr;


int cmp(const tree_record& t1, const tree_record& t2)
{
    int x = (int)t1.n_leaves() - (int)t2.n_leaves();

    if (x != 0) return x;

    x = (int)t1.n_internal_branches() - (int)t2.n_internal_branches();

    if (x != 0) return x;

    for(int i=0;i<t1.n_internal_branches();i++)
    {
	if (t1.partitions[i] == t2.partitions[i])
	    continue;
	else if (t1.partitions[i] < t2.partitions[i])
	    return -1;
	else
	    return 1;
    }
    return 0;
}

bool operator<(const tree_record& t1, const tree_record& t2)
{
    return cmp(t1,t2) < 0;
}

bool operator>(const tree_record& t1, const tree_record& t2)
{
    return cmp(t1,t2) > 0;
}

SequenceTree tree_sample::T(int i) const 
{
    return get_mf_tree(leaf_names,trees[i].partitions, trees[i].branch_lengths);
}

valarray<bool> tree_sample::support(const partition& p) const 
{
    valarray<bool> result(size());

    for(int i=0;i<result.size();i++) 
    {
	// Get a tree with the same topology
	const vector<dynamic_bitset<> > & T = trees[i].partitions;
    
	result[i] = implies(T,p);
    }
    return result;
}

valarray<bool> tree_sample::support(const vector<partition>& partitions) const 
{
    valarray<bool> result(size());

    vector<partition> informative_partitions = select(partitions,informative);

    for(int i=0;i<result.size();i++) 
    {
	// Get a tree with the same topology
	const vector<dynamic_bitset<> >& T = trees[i].partitions;
    
	result[i] = implies(T,informative_partitions);
    }
    return result;
}

unsigned tree_sample::count(const partition& P) const 
{
    unsigned count=0;
    for(int t=0;t<trees.size();t++) 
	if (implies(trees[t].partitions,P))
	    count ++;
   
    return count;
}

unsigned tree_sample::count(const vector<partition>& partitions) const 
{
    unsigned count=0;
    for(int t=0;t<trees.size();t++) {
	if (implies(trees[t].partitions,partitions))
	    count ++;
    }
   
    return count;
}

double tree_sample::PP(const partition& P) const 
{
    return double(count(P))/size();
}

double tree_sample::PP(const vector<partition>& partitions) const 
{
    return double(count(partitions))/size();
}

tree_record::tree_record(const Tree& T)
    :n_leaves_(T.n_leaves()),
     partitions(T.n_branches()-T.n_leafbranches()),
     branch_lengths(T.n_branches())
{ 
    vector<dynamic_bitset<> > temp(partitions.size());

    const int L = T.n_leafbranches();
    for(int i=L;i<T.n_branches();i++) {
	temp[i-L] = branch_partition(T,i);
	if (not temp[i-L][0])
	    temp[i-L].flip();
    }

    vector<int> order = iota<int>(partitions.size());
    std::sort(order.begin(),order.end(),sequence_order<dynamic_bitset<> >(temp));

    for(int i=0;i<L;i++)
    {
	if (T.branch(i).has_length())
	    branch_lengths[i] = T.branch(i).length();
	else
	    branch_lengths[i] = 1.0;
    }

    for(int i=L;i<T.n_branches();i++)
    {
	int o = order[i-L];
	partitions[i-L] = temp[o];
	if (T.branch(o+L).has_length())
	    branch_lengths[i] = T.branch(o+L).length();
	else
	    branch_lengths[i] = 1.0;
    }
}


void tree_sample::add_tree(const tree_record& T)
{
    trees.push_back(T);
}

void tree_sample::add_tree(Tree& T)
{
    //------------ check tree ---------------//
    if (has_sub_branches(T))
	throw myexception()<<"Tree has node of degree 2";

    // Compute the standardized representation
    T.standardize();
    add_tree(tree_record(T));
}

void tree_sample::add_tree(RootedTree& T)
{
    if (T.root().degree() == 2)
	T.remove_node_from_branch(T.root());

    add_tree(static_cast<Tree&>(T));
}

// What we actually want is a standardized STRING representation.
//  - We need to have ways of *reading* a Newick or NEXUS file and then
//  - ... converting to some intermediate record structure..
//  - ... which in this case in the standard string.

// We should be able to avoid sorting the leaves with every tree... in both cases!

// Reading an individual tree requires (a) one line, and (b) some state: ordered leaf names.

// Because we have to prune everything anyway, let's make the standard intermediate format
// .. be a (unrooted) Tree -- the names should be determined and given beforehand.



int tree_sample::load_file(istream& file,int skip,int subsample,int max,const vector<string>& prune)
{
    using namespace trees_format;

    //----------- Construct File Reader / Filter -----------//
    shared_ptr<reader_t> trees_in(new Newick_or_NEXUS(file));

    if (skip > 0)
	trees_in = shared_ptr<reader_t>(new Skip(skip,*trees_in));

    if (subsample > 1)
	trees_in = shared_ptr<reader_t>(new Subsample(subsample,*trees_in));

    if (max > 0)
	trees_in = shared_ptr<reader_t>(new Max(max,*trees_in));

    trees_in = shared_ptr<reader_t>(new Fixroot(*trees_in));

    if (prune.size())
	trees_in = shared_ptr<reader_t>(new Prune(prune,*trees_in));

    if (not leaf_names.size())
	leaf_names = trees_in->names();
    else 
    {
	vector<string> leaf_names2 = trees_in->names();
	if (leaf_names2.size() != leaf_names.size())
	    throw myexception()<<"New trees with "<<leaf_names2.size()<<" leaves conflict with current trees with "<<leaf_names.size()<<" leaves.";

	try {
	    compute_mapping(leaf_names, leaf_names2);
	}
	catch (bad_mapping<string>& b) {
	    throw myexception()<<"New trees are missing leaf '"<<b.missing<<"'";
	}

    }

    //------------------- Process Trees --------------------//
    RootedTree T;
    int t=0;
    while (trees_in->next_tree(T)) {
	add_tree(T);
	t++;
    }

    if (size() == 0)
	throw myexception()<<"No trees were read in!";

    return t;
}

int tree_sample::load_file(const string& filename,int skip,int subsample,int max,const vector<string>& prune)
{
    checked_ifstream file(filename,"tree samples file");
  
    int count = load_file(file,skip,subsample,max,prune);
    return count;
}

int tree_sample::append_trees(const tree_sample& trees)
{
    if (not leaf_names.size())
	leaf_names = trees.names();
    else 
    {
	if (trees.names().size() != leaf_names.size())
	    throw myexception()<<"New trees with "<<trees.names().size()<<" leaves conflict with current trees with "<<leaf_names.size()<<" leaves.";

	try {
	    compute_mapping(leaf_names, trees.names());
	}
	catch (bad_mapping<string>& b) {
	    throw myexception()<<"New trees are missing leaf '"<<b.missing<<"'";
	}

    }

    for(int i=0;i<trees.size();i++) {
	add_tree(trees[i]);
    }

    return trees.size();
}

tree_sample::tree_sample(istream& file,int skip,int subsample,int max,const vector<string>& prune)
{
    load_file(file,skip,subsample,max,prune);
}

tree_sample::tree_sample(const string& filename,int skip,int subsample,int max,const vector<string>& prune)
{
    load_file(filename,skip,subsample,max,prune);
}

void scan_trees(istream& file,int skip,int subsample,int max, const vector<string>& prune,
		const vector<string>& leaf_order, accumulator<SequenceTree>& op)
{
    using namespace trees_format;

    //----------- Construct File Reader / Filter -----------//
    shared_ptr<reader_t> trees(new Newick_or_NEXUS(file));

    if (skip > 0)
	trees = shared_ptr<reader_t>(new Skip(skip,*trees));

    if (subsample > 1)
	trees = shared_ptr<reader_t>(new Subsample(subsample,*trees));

    if (max > 0)
	trees = shared_ptr<reader_t>(new Max(max,*trees));

    trees = shared_ptr<reader_t>(new Fixroot(*trees));

    if (prune.size())
	trees = shared_ptr<reader_t>(new Prune(prune,*trees));

    if (leaf_order.size())
	trees = shared_ptr<reader_t>(new ReorderLeaves(leaf_order,*trees));

    //------------------- Process Trees --------------------//
    RootedSequenceTree T;
    while (trees->next_tree(T))
	op(T);

    //---------------------- Finalize ----------------------//
    op.finalize();
}

void scan_trees(istream& file,int skip,int subsample,int max, const vector<string>& prune,
		accumulator<SequenceTree>& op)
{
    scan_trees(file,skip,subsample,max,prune,vector<string>(),op);
}

void scan_trees(istream& file,int skip,int subsample,int max,
		accumulator<SequenceTree>& op)
{
    scan_trees(file,skip,subsample,max,vector<string>(),op);
}

