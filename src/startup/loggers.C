#include "loggers.H"
#include "tools/parsimony.H"
#include "computation/expression/expression.H"
#include "computation/expression/dummy.H"
#include "mcmc/mcmc.H"
#include "mcmc/logger.H"
#include "substitution/parsimony.H"

#include <set>

using std::vector;
using std::string;
using std::set;
using std::shared_ptr;
using std::make_shared;

using std::to_string;

/// Determine the parameters of model \a M that must be sorted in order to enforce identifiability.
vector< vector< vector<int> > > get_un_identifiable_indices(const Model& M, const vector<string>& names)
{
    if (not dynamic_cast<const Parameters*>(&M)) return {};

    const Parameters& P = dynamic_cast<const Parameters&>(M);
    vector< vector< vector<int> > > indices;

    int n_smodels = P.n_smodels();

    for(int i=0;i<n_smodels+1;i++) 
    {
	string prefix = "^";
	if (i>0)
	    prefix = string("S")+convertToString(i) + ".";

	vector< vector<int> > DP;
	if (parameters_with_extension(names, prefix + "DP.rate*").size()  )
	{
	    DP.push_back( parameters_with_extension(names, prefix + "DP.rate*") );
	    DP.push_back( parameters_with_extension(names, prefix + "DP.f*") );
	    indices.push_back( DP );
	}

	vector< vector<int> > M3;
	if (parameters_with_extension(names, prefix + "M3.omega*").size() )
	{
	    M3.push_back( parameters_with_extension(names, prefix + "M3.omega*") );
	    M3.push_back( parameters_with_extension(names, prefix + "M3.p*") );
	    indices.push_back( M3 );
	}
    }

    return indices;
}

void find_sub_loggers(Model& M, int& index, const string& name, vector<int>& logged_computations, vector<string>& logged_names)
{
    assert(index != -1);
    auto result = M.evaluate(index);
    if (result.is_double() or result.is_int())
    {
	logged_computations.push_back(index);
	logged_names.push_back(name);
	index = -1;
	return;
    }

    if (result.head().is_a<constructor>())
    {
	auto& c = result.head().as_<constructor>();
	if (c.f_name == "Prelude.True" or c.f_name == "Prelude.False")
	{
	    logged_computations.push_back(index);
	    logged_names.push_back(name);
	    index = -1;
	    return;
	}

	if (c.f_name == "[]")
	    return;

	if (c.f_name == ":")
	{
	    expression_ref L = M.get_expression(index);
	    expression_ref E = (dummy("Prelude.length"),L);
	    int length = M.evaluate_expression(E).as_int();
	    int index2 = -1;
	    for(int i=0;i<length;i++)
	    {
		expression_ref E2 = (dummy("Prelude.!!"),L,i) ;
		if (index2 == -1)
		    index2 = M.add_compute_expression(E2);
		else
		    M.set_compute_expression(index2, E2);

		find_sub_loggers(M, index2, name+"["+convertToString(i+1)+"]", logged_computations, logged_names);
	    }
	}
    }
}

MCMC::logger_function<std::string> construct_table_function(owned_ptr<Model>& M, const vector<string>& Rao_Blackwellize)
{
    owned_ptr<Parameters> P = M.as<Parameters>();

    using namespace MCMC;
    owned_ptr<TableGroupFunction<string> > TL = claim(new TableGroupFunction<string>);
  
    TL->add_field("iter", [](const Model&, long t) {return to_string(t);});
    TL->add_field("prior", [](const Model& M, long) {return to_string(log(M.prior()));});
    if (P)
	for(int i=0;i<P->n_data_partitions();i++)
	    if ((*P)[i].variable_alignment())
		TL->add_field("prior_A"+convertToString(i+1), [i](const Parameters& P) {return to_string(log(P[i].prior_alignment()));});
    TL->add_field("likelihood", [](const Model& M, long) {return to_string(log(M.likelihood()));});
    TL->add_field("logp", [](const Model& M, long) {return to_string(log(M.probability()));});
  
    {
	vector<int> logged_computations;
	vector<string> logged_names;

	vector<string> names_ = parameter_names(*M);
	set<string> names(names_.begin(), names_.end());

	// FIXME: Using short_parameter_names should be nice... but
	//          we are now logging EXPRESSIONS as well as actual parameters
	//        This makes such simplification difficult.

	for(int i=0;i<M->n_parameters();i++)
	{
	    string name = M->parameter_name(i);
	    if (name.size() and name[0] == '*' and not log_verbose) continue;

	    int index = M->add_compute_expression(parameter(name));

	    find_sub_loggers(*M, index, name, logged_computations, logged_names);
	}

	TableGroupFunction<expression_ref> T1;
	for(int i=0;i<logged_computations.size();i++)
	{
	    int index = logged_computations[i];
	    string name = logged_names[i];
	    T1.add_field(name, [index](const Model& M, long) {return get_computation(M,index);} );
	}

	SortedTableFunction T2(T1, get_un_identifiable_indices(*M, logged_names));

	TL->add_fields( ConvertTableToStringFunction<expression_ref>( T2 ) );
    }

    for(const auto& p: Rao_Blackwellize)
    {
	int p_index = M->find_parameter(p);
	if (p_index == -1)
	    throw myexception()<<"No such parameter '"<<p<<"' to Rao-Blackwellize";

	vector<expression_ref> values = {0,1};
	TL->add_field("RB-"+p, Get_Rao_Blackwellized_Parameter_Function(p_index, values));
    }

    if (not P or P->t().n_nodes() < 2) return TableLogger<string>(*TL);

    for(int i=0;i<P->n_data_partitions();i++)
    {
	if ((*P)[i].variable_alignment())
	{
	    TL->add_field("|A"+convertToString(i+1)+"|", [i](const Parameters& P){return to_string(alignment_length(P[i]));});
	    TL->add_field("#indels"+convertToString(i+1), [i](const Parameters& P){return to_string(n_indels(P[i]));});
	    TL->add_field("|indels"+convertToString(i+1)+"|", [i](const Parameters& P){return to_string(total_length_indels(P[i]));});
	}
	const alphabet& a = (*P)[i].get_alphabet();
	TL->add_field("#substs"+convertToString(i+1), [i,cost = unit_cost_matrix(a)](const Parameters& P) {return to_string(n_mutations(P[i],cost));});
	if (const Triplets* Tr = dynamic_cast<const Triplets*>(&a))
	    TL->add_field("#substs(nuc)"+convertToString(i+1), [i,cost = nucleotide_cost_matrix(*Tr)](const Parameters& P) {return to_string(n_mutations(P[i],cost));});
	if (const Codons* C = dynamic_cast<const Codons*>(&a))
	    TL->add_field("#substs(aa)"+convertToString(i+1), [i,cost = amino_acid_cost_matrix(*C)](const Parameters& P) {return to_string(n_mutations(P[i],cost));});
    }

    // Add fields Scale<s>*|T|
    for(int i=0;i<P->n_branch_scales();i++)
    {
	auto name = string("Scale")+to_string(i+1)+"*|T|";

	auto f = [i](const Parameters& P) {return to_string( P.branch_scale(i)*tree_length(P.t()));};

	TL->add_field(name, f);
    }
  
    if (P->variable_alignment()) {
	TL->add_field("|A|", Get_Total_Alignment_Length_Function() );
	TL->add_field("#indels", Get_Total_Num_Indels_Function() );
	TL->add_field("|indels|", Get_Total_Total_Length_Indels_Function() );
    }
    TL->add_field("#substs", Get_Total_Num_Substitutions_Function() );
  
    TL->add_field("|T|", Get_Tree_Length_Function() );

    return TableLogger<string>(*TL);
}

vector<MCMC::Logger> construct_loggers(owned_ptr<Model>& M, int subsample, const vector<string>& Rao_Blackwellize, int proc_id, const string& dir_name)
{
    using namespace MCMC;
    vector<Logger> loggers;

    owned_ptr<Parameters> P = M.as<Parameters>();

    string base = dir_name + "/" + "C" + convertToString(proc_id+1);

    logger_function<string> TF = Subsample_Function(construct_table_function(M, Rao_Blackwellize),subsample);

    Logger s = FunctionLogger(base +".p", TF);
  
    // Write out scalar numerical variables (and functions of them) to C<>.p
    loggers.push_back( s );
  
    if (not P) return loggers;

    // Write out the (scaled) tree each iteration to C<>.trees
    loggers.push_back( FunctionLogger(base + ".trees", TreeFunction()<<"\n" ) );
  
    // Write out the MAP point to C<>.MAP - later change to a dump format that could be reloaded?
    {
	ConcatFunction F; 
	F<<TF<<"\n";
	F<<Show_SModels_Function()<<"\n";
	if (P->t().n_nodes() > 1)
	    for(int i=0;i<P->n_data_partitions();i++)
		if ((*P)[i].variable_alignment())
		    F<<AlignmentFunction(i)<<"\n\n";
	F<<TreeFunction()<<"\n\n";
	loggers.push_back( FunctionLogger(base + ".MAP", MAP_Function(F)) );
    }

    // Write out the probability that each column is in a particular substitution component to C<>.P<>.CAT
    if (P->contains_key("log-categories"))
	for(int i=0;i<P->n_data_partitions();i++)
	    loggers.push_back( FunctionLogger(base + ".P" + convertToString(i+1)+".CAT", 
					      Mixture_Components_Function(i) ) );

    // Write out ancestral sequences
    if (P->contains_key("log-ancestral") and P->t().n_nodes() > 1)
	for(int i=0;i<P->n_data_partitions();i++)
	    loggers.push_back( FunctionLogger(base + ".P" + convertToString(i+1)+".ancestral.fastas", 
								   Ancestral_Sequences_Function(i) ) );


    // Write out the alignments for each (variable) partition to C<>.P<>.fastas
    if (P->t().n_nodes() > 1)
	for(int i=0;i<P->n_data_partitions();i++)
	    if ((*P)[i].variable_alignment()) 
	    {
		string filename = base + ".P" + convertToString(i+1)+".fastas";
		
		ConcatFunction F;
		auto iterations = [](const Model&, long t) {return to_string(t);};
		F<<"iterations = "<<iterations<<"\n\n";
		F<<AlignmentFunction(i);
		
		loggers.push_back( FunctionLogger(filename, Subsample_Function(F,10) ) );
	    }

    return loggers;
}

