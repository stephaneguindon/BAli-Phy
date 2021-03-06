#ifndef SMODEL_SETUP_H
#define SMODEL_SETUP_H
#include <vector>
#include <valarray>
#include <string>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include "object.H"
#include "alignment/alignment.H"
#include "sequence/alphabet.H"
#include "computation/expression/expression_ref.H"

struct model_t
{
    boost::property_tree::ptree description;
    boost::property_tree::ptree type;
    expression_ref expression;
};

model_t get_model(const boost::property_tree::ptree& type, const boost::property_tree::ptree& model_rep);

model_t get_model(const std::string& type, const std::string& model);

std::string default_markov_model(const alphabet& a);

std::string show(const boost::property_tree::ptree& pt, int depth = 0);
#endif
