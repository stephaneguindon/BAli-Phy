/*
  Copyright (C) 2004-2006,2009-2012 Benjamin Redelings

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

#include <set>
#include <map>

#include "util.H"
#include "myexception.H"
#include "models/model.H"
#include "computation/program.H"
#include "computation/expression/expression.H"
#include "computation/module.H"
#include "parser/desugar.H"

using std::vector;
using std::string;

using boost::dynamic_pointer_cast;

vector<expression_ref> model_parameter_expressions(const Model& M)
{
    vector< expression_ref > sub;
    for(int i=0;i<M.n_parameters();i++) 
	sub.push_back( parameter(M.parameter_name(i)) );
    return sub;
}

std::vector< expression_ref > Model::get_parameter_values() const
{
    std::vector< expression_ref > values(n_parameters());

    for(int i=0;i<values.size();i++)
	values[i] = get_parameter_value(i);
    
    return values;  
}

std::vector< expression_ref > Model::get_parameter_values(const std::vector<int>& indices) const
{
    std::vector< expression_ref > values(indices.size());
    
    for(int i=0;i<values.size();i++)
	values[i] = get_parameter_value(indices[i]);
  
    return values;  
}

bool Model::has_bounds(int i) const 
{
    auto e = get_parameter_range(i);

    return (e and e.is_a<Bounds<double>>());
}

const Bounds<double>& Model::get_bounds(int i) const 
{
    auto e = get_parameter_range(i);

    assert(e);

    if (not e.is_a<Bounds<double>>())
	throw myexception()<<"parameter '"<<parameter_name(i)<<"' doesn't have Bounds<double>.";

    return e.as_<Bounds<double>>();
}

std::vector< expression_ref > Model::get_modifiable_values(const std::vector<int>& indices) const
{
    std::vector< expression_ref > values(indices.size());
    
    for(int i=0;i<values.size();i++)
	values[i] = get_modifiable_value(indices[i]);
  
    return values;  
}

void Model::set_modifiable_value(int m, const expression_ref& value) 
{
    context::set_modifiable_value(m, value);
    recalc();
}

void Model::set_parameter_value(int i,const expression_ref& value) 
{
    set_parameter_values({i}, {value});
}

void Model::set_parameter_value(const string& p_name,const expression_ref& value) 
{
    int i = find_parameter(p_name);
    if (i == -1)
	throw myexception()<<"Cannot find parameter called '"<<p_name<<"'";
    
    set_parameter_values({i},{value});
}

void Model::set_parameter_values(const vector<int>& indices,const vector<expression_ref>& p)
{
    assert(indices.size() == p.size());

    for(int i=0;i<indices.size();i++)
	context::set_parameter_value(indices[i], p[i]);

    recalc();
}

log_double_t Model::prior() const
{
    return get_probability();
}

Model::Model(const std::shared_ptr<module_loader>& L)
    :context(L),keys(new std::map<std::string,double>)
{ }

void show_parameters(std::ostream& o,const Model& M, bool show_hidden) {
    for(int i=0;i<M.n_parameters();i++) {
	string name = M.parameter_name(i);
	if ((not show_hidden) and name.size() > 1 and name[0] == '*')
	    continue;
	o<<"    ";
	o<<name<<" = ";
	string output="[NULL]";
	if (M.get_parameter_value(i))
	{
	    output=M.get_parameter_value(i).print();
	    if (output.find(10) != string::npos or output.find(13) != string::npos)
		output = "[multiline]";
	}
	o<<output;
    }
    o<<"\n";
}

std::string show_parameters(const Model& M, bool show_hidden)
{
    std::ostringstream oss;
    show_parameters(oss,M, show_hidden);
    return oss.str();
}

/// \brief Check if the model M has a parameter called name
///
/// \param M      The model
/// \param name   A parameter name
///
bool has_parameter(const Model& M, const string& name)
{
    for(int i=0;i<M.n_parameters();i++)
	if (M.parameter_name(i) == name)
	    return true;
    return false;
}

/// \brief Check if the string s1 matches a pattern s2
///
/// \param s1   The string
/// \param s2   The pattern
///
bool pattern_match(const string& s1, const string& s2)
{
    if (s2.size() and s2[s2.size()-1] == '*') {
	int L = s2.size() - 1;
	if (L > s1.size()) return false;
	return (s1.substr(0,L) == s2.substr(0,L));
    }
    else
	return s1 == s2;
}

bool operator<(const vector<string>& p1, const vector<string>& p2)
{
    // less than
    if (p1.size() > p2.size()) return true;
    // greater than
    if (p1.size() < p2.size()) return false;
  
    for(int i=0;i<p1.size();i++)
    {
	int cmp = p1[i].compare(p2[i]);
	// less than
	if (cmp < 0) return true;
	// greater than
	if (cmp > 0) return false;
    }
  
    // equal
    return false;
}

typedef std::set< vector<string> > path_set_t;

/// Does this path have the given prefix?
bool path_has_prefix(const vector<string>& path, const vector<string>& path_prefix)
{
    if (path_prefix.size() > path.size()) return false;

    for(int i=0;i<path_prefix.size();i++)
	if (path[i] != path_prefix[i])
	    return false;

    return true;
}

/// Are the paths all distinguishable from each other?
bool overlap(const path_set_t& set1, const path_set_t& set2)
{
    if (set1.empty() or set2.empty()) return false;

    path_set_t::const_iterator it1 = set1.begin(), it1End = set1.end();
    path_set_t::const_iterator it2 = set2.begin(), it2End = set2.end();

    if(*it1 > *set2.rbegin() || *it2 > *set1.rbegin()) return false;

    while(it1 != it1End && it2 != it2End)
    {
	if(*it1 < *it2)
	    it1++; 
	else if (*it1 > *it2)
	    it2++; 
	else
	    return true;
    }

    return false;
}

/// Remove the nodes in paths that are direct children of the path_prefix
void remove_prefix(vector< vector<string> >& paths, const  vector<string>& path_prefix)
{
    for(int i=0;i<paths.size();i++)
    {
	if (not path_has_prefix(paths[i], path_prefix)) continue;

	paths[i].erase(paths[i].begin()+path_prefix.size()-1);
    }
}

/// Remove (internal) child paths if grandchild paths are not shared with any other child.
void check_remove_grandchildren(vector< vector<string> >& paths, const vector<string>& path_prefix)
{
    // construct the child paths and their locations
    typedef std::map<string, path_set_t> path_map_t;
    path_map_t grandchild_paths;

    int L = path_prefix.size();

    // find the grandchild paths for each child
    for(int i=0;i<paths.size();i++)
	if (path_has_prefix(paths[i], path_prefix))
	{
	    // We don't consider leaf child paths or empty paths
	    if (paths[i].size() <= path_prefix.size() + 1)
		continue;

	    string child_name = paths[i][L];

	    vector<string> grandchild_path = paths[i];
	    grandchild_path.erase(grandchild_path.begin(),grandchild_path.begin()+L+1);

	    grandchild_paths[child_name].insert(grandchild_path);
	    assert(grandchild_path.size());
	}

    // check of the grandchild paths of any child overlap with the grandchild paths of any other child
    for(path_map_t::const_iterator i = grandchild_paths.begin();i != grandchild_paths.end();i++)
    {
	bool unique = true;
	for(path_map_t::const_iterator j = grandchild_paths.begin();j != grandchild_paths.end();j++)
	{
	    if (i->first == j->first) continue;

	    if (overlap(i->second,j->second)) unique = false;
	}
	if (unique) {
	    vector<string> child_prefix = path_prefix;
	    child_prefix.push_back(i->first);
	    remove_prefix(paths, child_prefix);
	}
    }
}

// We can think of this collection of name lists as a tree.
// - Each name list is a path from the root to a tip.
// - Each node (except the root) has a string name associated with it.
// We consider all child nodes of internal node
//  If the set of grandchild lists under child node C does not overlap with the
//   grandchild lists under any other child node, then we can remove node C.
// We should always prefer to remove deeper nodes first.
//  Thus, leaf nodes should never be removed.
// We therefore consider all internal nodes of the tree, starting
//  with the ones furthest from the root, and remove their children
//  if it is allowable.

vector<string> short_parameter_names(vector<string> names)
{
    // for any sequence n[0] n[1] ... n[i-1] n[i] n[i+1] ..... N[L]
    // If we select all the sequences where where  n[0].... n[i-1] are the same
    //  Then we can get rid of n[i] if the sequences n[i+1]...N[L] are all different

    // construct the name paths
    vector< vector<string> > paths;
    for(int i=0;i<names.size();i++)
	paths.push_back(split(names[i],"."));

    for(int i=0;i<paths.size();i++)
    {
	vector<string> prefix = paths[i];
	prefix.pop_back();
	while(prefix.size())
	{
	    check_remove_grandchildren(paths, prefix);
	    prefix.pop_back();
	}
    }
  
    for(int i=0;i<names.size();i++)
	names[i] = join(paths[i],".");

    return names;
}

vector<string> parameter_names(const Model& M)
{
    vector<string> names;
    for(int i=0;i<M.n_parameters();i++)
	names.push_back( M.parameter_name(i) );

    return names;
}

vector<string> short_parameter_names(const Model& M)
{
    return short_parameter_names( parameter_names( M ) );
}

// The key may have '*', '?', 'text*', and 'text'.  Here
// - '?' matches exactly one element.
// - '*' matches 0 or more elements
// - text* matches 1 element beginning with 'text'
// - test matches 1 element that is exactly 'text'.
bool path_match(const vector<string>& key, int i, const vector<string>& pattern, int j)
{
    assert(i <= key.size());

    assert(j <= pattern.size());

    // 1. If the key is empty
    if (i==key.size())
    {
	// nothing ~ nothing
	if(j==pattern.size()) return true;

	// nothing !~ something
	if (j<pattern.size()) return false;
    }

    assert(i<key.size());

    // 2. If the key is '*'
    if (key[i] == "*") 
    {
	// (* x y z ~ [nothing]) only (if x y z ~ [nothing])
	if (j == pattern.size())
	    return path_match(key,i+1,pattern,j);

	// (* matches 0 components of j) or (* matches 1 or more components of j)
	return path_match(key,i+1,pattern,j) or path_match(key,i,pattern,j+1);
    }

    // For anything else, FAIL if we are matching against [nothing], and key != '*' only.
    if (j == pattern.size()) return false;

    assert(j<pattern.size());

    // 3. If the key is '?'
    if (key[i] == "?")
	return path_match(key,i+1,pattern,j+1);

    // 4. If the key is 'text*' or 'text'
    if (pattern_match(pattern[j],key[i]))
	return path_match(key,i+1,pattern,j+1);
    else
	return false;
}

bool path_match(const vector<string>& key, const vector<string>& pattern)
{
    return path_match(key,0,pattern,0);
}

/// \brief Find the index of model parameters that match the pattern name
///
/// \param M      The model
/// \param name   The pattern
///
vector<int> parameters_with_extension(const vector<string>& M, string name)
{
    vector<int> indices;

    const vector<string> key = split(name,".");

    if (not key.size()) return indices;

    vector<string> skeleton;

    for(int i=0;i<M.size();i++)
    {
	vector<string> pattern = split(M[i],".");

	if (not path_match(key, pattern)) continue;

	indices.push_back(i);
    }

    return indices;
}

/// \brief Find the index of model parameters that match the pattern name
///
/// \param M      The model
/// \param name   The pattern
///
vector<int> parameters_with_extension(const Model& M, string name)
{
    return parameters_with_extension(parameter_names(M), name);
}

