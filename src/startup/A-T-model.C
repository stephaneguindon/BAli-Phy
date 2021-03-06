#include "A-T-model.H"
#include "io.H"
#include "models/setup.H"
#include "tree/tree-util.H" //extends
#include "alignment/alignment-constraint.H"
#include "alignment/alignment-util.H"
#include "models/parse.H"

namespace po = boost::program_options;
using po::variables_map;

using boost::dynamic_bitset;

using std::cout;
using std::cerr;
using std::clog;
using std::endl;
using std::ostream;
using std::string;
using std::vector;

/// Replace negative or zero branch lengths with saner values.
void sanitize_branch_lengths(SequenceTree& T)
{
    double min_branch = 0.000001;
    for(int i=0;i<T.n_branches();i++)
    {
	if (not T.branch(i).has_length())
	    T.branch(i).set_length(3.0/T.n_branches());
	if (T.branch(i).length() > 0)
	    min_branch = std::min(min_branch,T.branch(i).length());
    }
  
    for(int i=0;i<T.n_branches();i++) {
	if (T.branch(i).length() == 0)
	    T.branch(i).set_length(min_branch);
	if (T.branch(i).length() < 0)
	    T.branch(i).set_length( - T.branch(i).length() );
    }
}

vector<double> get_geometric_heating_levels(const string& s)
{
    vector<double> levels;

    vector<string> parse = split(s,'/');

    if (parse.size() != 2) return levels;

    try
    {
	int n_levels = convertTo<int>(parse[1]);
	levels.resize(n_levels);
    
	parse = split(parse[0],'-');
	levels[0] = convertTo<double>(parse[0]);
	levels.back() = convertTo<double>(parse[1]);
	double factor = pow(levels.back()/levels[0], 1.0/(n_levels-1));
    
	for(int i=1;i<levels.size()-1;i++)
	    levels[i] = levels[i-1]*factor;
    
	return levels;
    }
    catch (...)
    {
	throw myexception()<<"I don't understand beta level string '"<<s<<"'";
    }
}


void setup_heating(int proc_id, const variables_map& args, Parameters& P) 
{
    if (args.count("beta")) 
    {
	string beta_s = args["beta"].as<string>();

	vector<double> beta = get_geometric_heating_levels(beta_s);
	if (not beta.size())
	    beta = split<double>(beta_s,',');

	P.PC->all_betas = beta;

	if (proc_id >= beta.size())
	    throw myexception()<<"not enough temperatures given: only got "<<beta.size()<<", wanted at least "<<proc_id+1;

	P.beta_index = proc_id;

	P.set_beta(beta[proc_id]);

	P.PC->beta_series.push_back(beta[proc_id]);
    }

    if (args.count("dbeta")) {
	vector<string> deltas = split(args["dbeta"].as<string>(),',');
	for(int i=0;i<deltas.size();i++) {
	    vector<double> D = split<double>(deltas[i],'*');
	    if (D.size() != 2)
		throw myexception()<<"Couldn't parse beta increment '"<<deltas[i]<<"'";
	    int D1 = (int)D[0];
	    double D2 = D[1];
	    for(int i=0;i<D1;i++) {
		double next = P.PC->beta_series.back() + D2;
		next = std::max(0.0,next);
		P.PC->beta_series.push_back(next);
	    }
	}
    }
    for(double b:P.PC->beta_series)
	std::cout<<b<<"\n";
}

vector<model_t>
get_imodels(const shared_items<string>& imodel_names_mapping, const SequenceTree&)
{
    vector<model_t> imodels;
    for(int i=0;i<imodel_names_mapping.n_unique_items();i++) 
	imodels.push_back( get_model("IM",imodel_names_mapping.unique(i)) );
    return imodels;
}

string show_model(boost::property_tree::ptree p)
{
    bool top_sample = false;
    if (p.get_value<string>() == "Sample")
    {
	top_sample = true;
	p = p.begin()->second;
    }

    string output = unparse(p);
    string connector = top_sample?"~ ":"= ";

    return connector + output;
}

void log_summary(ostream& out_cache, ostream& out_screen,ostream& /* out_both */,
		 const vector<model_t>& IModels, const vector<model_t>& SModels,
		 const vector<model_t>& ScaleModels,
		 const model_t& branch_length_model,
		 const Parameters& P,const variables_map& args)
{
    //-------- Log some stuff -----------//
    vector<string> filenames = args["align"].as<vector<string> >();
    for(int i=0;i<filenames.size();i++) {
	out_cache<<"data"<<i+1<<" = "<<filenames[i]<<endl<<endl;
	out_cache<<"alphabet"<<i+1<<" = "<<P[i].get_alphabet().name<<endl<<endl;
    }

    for(int i=0;i<P.n_data_partitions();i++) {
	out_cache<<"smodel-index"<<i+1<<" = "<<P.smodel_index_for_partition(i)<<endl;
	out_cache<<"imodel-index"<<i+1<<" = "<<P.imodel_index_for_partition(i)<<endl;
	out_cache<<"scale-index"<<i+1<<" = "<<P.scale_index_for_partition(i)<<endl;
    }
    out_cache<<endl;

    for(int i=0;i<P.n_smodels();i++)
	//    out_cache<<"subst model"<<i+1<<" = "<<P.SModel(i).name()<<endl<<endl;
	out_cache<<"subst model"<<i+1<<" "<<show_model(SModels[i].description)<<endl<<endl;

    for(int i=0;i<P.n_imodels();i++)
	out_cache<<"indel model"<<i+1<<" "<<show_model(IModels[i].description)<<endl<<endl;

    for(int i=0;i<P.n_branch_scales();i++)
	out_cache<<"scale model"<<i+1<<" "<<show_model(ScaleModels[i].description)<<endl<<endl;

    if (P.t().n_branches() > 1)
	out_screen<<"T:topology ~ uniform on tree topologies\n";

    if (P.t().n_branches() > 0)
	out_screen<<"T:length[b] "<<show_model(branch_length_model.description)<<endl<<endl;

    for(int i=0;i<P.n_data_partitions();i++) {
	int s_index = P.smodel_index_for_partition(i);
	//    out_screen<<"#"<<i+1<<": subst ~ "<<P.SModel(s_index).name()<<" ("<<s_index+1<<")    ";
	out_screen<<"#"<<i+1<<": subst "<<show_model(SModels[s_index].description)<<" (S"<<s_index+1<<")\n";

	int i_index = P.imodel_index_for_partition(i);
	string i_name = " = none";
	if (i_index != -1)
	    i_name = show_model(IModels[i_index].description);
	out_screen<<"#"<<i+1<<": indel "<<i_name;
	if (i_index >= 0)
	    out_screen<<" (I"<<i_index+1<<")";
	out_screen<<endl;

	int scale_index = P.scale_index_for_partition(i);
	out_screen<<"#"<<i+1<<": scale "<<show_model(ScaleModels[scale_index].description)<<" (Scale"<<scale_index+1<<")\n";
	out_screen<<endl;

	if (log_verbose)
	    out_screen<<show(SModels[i].description);
    }
}

void check_alignment_names(const alignment& A)
{
    const string forbidden = "();:\"'[]&,";

    for(int i=0;i<A.n_sequences();i++) {
	const string& name = A.seq(i).name;
	for(int j=0;j<name.size();j++)
	    for(int c=0;c<forbidden.size();c++)
		for(int pos=0;pos<name.size();pos++)
		    if (name[pos] == forbidden[c])
			throw myexception()<<"Sequence name '"<<name<<"' contains illegal character '"<<forbidden[c]<<"'";
    }
}

void check_alignment_values(const alignment& A,const string& filename)
{
    const alphabet& a = A.get_alphabet();

    for(int i=0;i<A.n_sequences();i++)
    {
	string name = A.seq(i).name;

	for(int j=0;j<A.length();j++) 
	    if (A.unknown(j,i))
		throw myexception()<<"Alignment file '"<<filename<<"' has a '"<<a.unknown_letter<<"' in sequence '"<<name<<"'.\n (Please replace with gap character '"<<a.gap_letter<<"' or wildcard '"<<a.wildcard<<"'.)";
    }
}

/// If the tree has any foreground branch attributes, then set the corresponding branch to foreground, here.
void set_foreground_branches(Parameters& P, const SequenceTree& T)
{
    if (T.find_undirected_branch_attribute_index_by_name("foreground") != -1)
    {
	int attribute_index = T.find_undirected_branch_attribute_index_by_name("foreground");

	for(int b=0;b<T.n_branches();b++)
	{
	    boost::any value = T.branch(b).undirected_attribute(attribute_index);
	    if (value.empty()) continue;

	    int foreground_level = convertTo<int>( boost::any_cast<string>( value) );

	    P.set_parameter_value( P.find_parameter("*Main.branchCat"+convertToString(b+1)), foreground_level);
	    std::cerr<<"Setting branch '"<<b<<"' to foreground level "<<foreground_level<<"\n";;
	}
    }
}

void write_branch_numbers(ostream& o, SequenceTree T)
{
    // Write out a tree 
    auto flags = o.flags();
    o.unsetf(std::ios::floatfield);
    for(int b=0;b<T.n_branches();b++)
	T.branch(b).set_length(b);
    o<<"branch numbers = "<<T<<"\n\n";
    o.flags(flags);
}

// return the list of constrained branches
vector<int> load_alignment_branch_constraints(const string& filename, const SequenceTree& TC)
{
    // open file
    checked_ifstream file(filename,"alignment-branch constraint file");

    // read file
    string line;
    vector<vector<string> > name_groups;
    while(portable_getline(file,line)) {
	vector<string> names = split(line,' ');
	for(int i=names.size()-1;i>=0;i--)
	    if (names[i].size() == 0)
		names.erase(names.begin()+i);

	if (names.size() == 0) 
	    continue;
	else if (names.size() == 1)
	    throw myexception()<<"In alignment constraint file: you must specify more than one sequence per group.";
    
	name_groups.push_back(names);
    }

    // parse the groups into mask_groups;
    vector< dynamic_bitset<> > mask_groups(name_groups.size());
    for(int i=0;i<mask_groups.size();i++) 
    {
	mask_groups[i].resize(TC.n_leaves());
	mask_groups[i].reset();

	for(int j=0;j<name_groups[i].size();j++) 
	{
	    int index = find_index(TC.get_leaf_labels(), name_groups[i][j]);

	    if (index == -1)
		throw myexception()<<"Reading alignment constraint file '"<<filename<<"':\n"
				   <<"   Can't find leaf taxon '"<<name_groups[i][j]<<"' in the tree.";
	    else
		mask_groups[i][index] = true;
	}
    }

    // 1. check that each group is a fully resolved clade in the constraint tree (no polytomies)
    // 2. construct the list of constrained branches
    // FIXME - what if the user specifies nested clades?  Won't we get branches twice, then?
    //       - SOLUTION: use a bitmask.
    vector<int> branches;
    for(int i=0;i<mask_groups.size();i++) 
    {
	// find the branch that corresponds to a mask
	boost::dynamic_bitset<> mask(TC.n_leaves());
	int found = -1;
	for(int b=0;b<2*TC.n_branches() and found == -1;b++) 
	{
	    mask = TC.partition(b);

	    if (mask_groups[i] == mask)
		found = b;
	}

	// complain if we can't find it
	if (found == -1) 
	    throw myexception()<<"Alignment constraint: clade '"
			       <<join(name_groups[i],' ')
			       <<"' not found in topology constraint tree.";
    
	// mark branch and child branches as constrained
	vector<const_branchview> b2 = branches_after_inclusive(TC,found); 
	for(int j=0;j<b2.size();j++) {
	    if (b2[j].target().degree() > 3)
		throw myexception()<<"Alignment constraint: clade '"
				   <<join(name_groups[i],' ')
				   <<"' has a polytomy in the topology constraint tree.";
	    branches.push_back(b2[j].undirected_name());
	}
    }


    return branches;
}


owned_ptr<Model> create_A_and_T_model(variables_map& args, const std::shared_ptr<module_loader>& L,
				      ostream& out_cache, ostream& out_screen, ostream& out_both,
				      int proc_id)
{
    //------ Determine number of partitions ------//
    vector<string> filenames = args["align"].as<vector<string> >();
    const int n_partitions = filenames.size();

    //-------------Choose an indel model--------------//
    //FIXME - make a shared_items-like class that also holds the items to we can put the whole state in one object.
    //FIXME - make bali-phy.C a more focussed and readable file - remove setup junk to other places? (where?)
    vector<int> imodel_mapping(n_partitions, -1);
    shared_items<string> imodel_names_mapping(vector<string>(),imodel_mapping);

    imodel_names_mapping = get_mapping(args, "imodel", n_partitions);

    for(int i=0;i<imodel_names_mapping.n_unique_items();i++)
	if (imodel_names_mapping.unique(i) == "")
	    imodel_names_mapping.unique(i) = "RS07";
      
    imodel_mapping = imodel_names_mapping.item_for_partition;

    //------------- Get smodel names -------------------
    shared_items<string> smodel_names_mapping = get_mapping(args, "smodel", n_partitions);

    vector<int> smodel_mapping = smodel_names_mapping.item_for_partition;

    vector<model_t> full_smodels(smodel_names_mapping.n_unique_items());

    for(int i=0;i<smodel_names_mapping.n_unique_items();i++)
	if (not smodel_names_mapping.unique(i).empty())
	    full_smodels[i] = get_model("MMM[a]",smodel_names_mapping.unique(i));

    //------------- Get alphabet names -------------------
    shared_items<string> alphabet_names_mapping = get_mapping(args, "alphabet", filenames.size());
    vector<string> alphabet_names;
    for(int i=0;i<alphabet_names_mapping.n_partitions();i++)
	alphabet_names.push_back(alphabet_names_mapping[i]);

    for(int i=0;i<smodel_names_mapping.n_unique_items();i++)
    {
	if (smodel_names_mapping.unique(i).empty()) continue;

	auto type = full_smodels[i].type;
	auto alphabet_type = type.begin()->second;

	vector<int> a_specified;
	vector<int> a_unspecified;
	for(int j: smodel_names_mapping.partitions_for_item[i])
	{
	    if (alphabet_names[j].empty())
		a_unspecified.push_back(j);
	    else
		a_specified.push_back(j);
	}
	if (a_specified.size() and a_unspecified.size())
	    throw myexception()<<"SModel "<<i+1<<" applied to partition "<<a_specified[i]+1<<" (alphabet specified) and partition "
			       <<a_unspecified[0]+1<<" (alphabet not specified).";

	if (a_specified.size())
	{
	    for(int j: a_specified)
		if (alphabet_names[j] != alphabet_names[a_specified[0]])
		    throw myexception()<<"Partitions "<<a_specified[0]+1<<" and "<<j+1<<" have different alphabets, but are given the same substitution model!";
	    string a = alphabet_names[a_specified[0]];
	    if (alphabet_type.get_value<string>() == "Codon")
	    {
		if (a == "DNA" or a == "RNA" or a == "AA" or a == "Amino-Acids")
		    throw myexception()<<"Partition "<<a_specified[0]+1<<" has specified alphabet '"<<a<<"' but the substitution model requires a codon alphabet!";
	    }
	    else if (alphabet_type.get_value<string>() == "AA")
	    {
		if (a != "AA" and a != "Amino-Acids" and a != "Amino-AcidsWithStop")
		    throw myexception()<<"Partition "<<a_specified[0]+1<<" has specified alphabet '"<<a<<"' but the substitution model requires an amino-acid alphabet!";
	    }
	}
	else if (alphabet_type.get_value<string>() == "Codon")
	{
	    for(int j: smodel_names_mapping.partitions_for_item[i])
		alphabet_names[j] = "Codons";
	}
//      Use the auto-detected alphabet right now -- it leaves to better error messages.
//	else if (alphabet_type.get_value<string>() == "AA")
//	{
//	    for(int j: smodel_names_mapping.partitions_for_item[i])
//		alphabet_names[j] = "AA";
//	}
    }

    //----------- Load alignments  ---------//
    vector<alignment> A(filenames.size());

    // -- load alphabets with specified names
    for(int i=0;i<filenames.size();i++)
	if (alphabet_names[i].size())
	    A[i] = load_alignment(filenames[i], load_alphabets(alphabet_names[i]) );

    // -- load other alphabets
    for(int i=0;i<filenames.size();i++)
	if (not A[i].has_alphabet())
	    A[i] = load_alignment(filenames[i]);

    for(int i=0;i<A.size();i++) {
	check_alignment_names(A[i]);
	check_alignment_values(A[i],filenames[i]);
    }

    //----------- Load tree and link to alignments ---------//
    SequenceTree T;

    if (args.count("tree"))
	T = load_T(args);
    else
    {
	SequenceTree TC = star_tree(sequence_names(A[0]));
	if (args.count("t-constraint"))
	    TC = load_constraint_tree(args["t-constraint"].as<string>(),sequence_names(A[0]));

	T = TC;
	RandomTree(T, 1.0);
    }

    for(auto& a: A)
	link(a,T,true);

    //--------- Handle branch lengths <= 0 --------//
    sanitize_branch_lengths(T);

    //--------- Set up indel model --------//
    auto full_imodels = get_imodels(imodel_names_mapping, T);

    //--------- Get substitution models that depend on default alphabet --------//
    for(int i=0;i<smodel_names_mapping.n_unique_items();i++)
	if (smodel_names_mapping.unique(i).empty())
	{
	    int first_index = smodel_names_mapping.partitions_for_item[i][0];
	    const alphabet& a = A[first_index].get_alphabet();

	    smodel_names_mapping.unique(i) = default_markov_model(a);

	    if (smodel_names_mapping.unique(i) == "")
		throw myexception()<<"You must specify a substitution model - there is no default substitution model for alphabet '"<<a.name<<"'";

	    full_smodels[i] = get_model("MMM[a]",smodel_names_mapping.unique(i));
	}
    
    // Apply alphabet
    for(int i=0;i<smodel_names_mapping.n_unique_items();i++)
    {
	int first_index = smodel_names_mapping.partitions_for_item[i][0];
	const alphabet& a = A[first_index].get_alphabet();

	auto type = full_smodels[i].type;
	auto alphabet_type = type.begin()->second;

	if (alphabet_type.get_value<string>() == "Codon" and not dynamic_cast<const Codons*>(&a))
	    throw myexception()<<"Substitution model S"<<i+1<<" requires a codon alphabet, but sequences are '"<<a.name<<"'";;

	if (alphabet_type.get_value<string>() == "AA" and not dynamic_cast<const AminoAcids*>(&a))
	    throw myexception()<<"Substitution model S"<<i+1<<" requires an amino-acid alphabet, but sequences are '"<<a.name<<"'";;

	full_smodels[i].expression = (full_smodels[i].expression, a);
    }

    //-------------- Which partitions share a scale? -----------//
    shared_items<string> scale_names_mapping = get_mapping(args, "scale", A.size());

    vector<int> scale_mapping = scale_names_mapping.item_for_partition;

    vector<model_t> full_scale_models(scale_names_mapping.n_unique_items());

    for(int i=0;i<scale_names_mapping.n_unique_items();i++)
    {
	auto scale_model = scale_names_mapping.unique(i);
	if (scale_model.empty())
	    scale_model = "~Gamma[0.5,2]";
	full_scale_models[i] = get_model("Double", scale_model);
    }

    //-------------- Branch length model --------------------//
    model_t branch_length_model;
    if (args.count("branch-length"))
	branch_length_model = get_model("Double", args["branch-length"].as<string>());

    // Don't divide by 0 if we have 1 sequence and T.n_branches() == 0
    else if (T.n_branches() > 0)
    {
	string beta = std::to_string(2.0/T.n_branches());
	branch_length_model = get_model("Double", string("~Gamma[0.5,")+beta+"]");
    }

    //--------------- Create the Parameters object---------------//
    Parameters P(L, A, T, full_smodels, smodel_mapping, full_imodels, imodel_mapping, full_scale_models, scale_mapping, branch_length_model);

    //-------- Set the alignments for variable partitions ---------//
    bool unalign = args.count("unalign");
    for(int i=0;i<P.n_data_partitions();i++)
	if (P.get_data_partition(i).has_IModel() and not unalign)
	    P.get_data_partition(i).set_alignment(A[i]);

    // If the tree has any foreground branch attributes, then set the corresponding branch to foreground, here.
    set_foreground_branches(P, T);

    //------------- Write out a tree with branch numbers as branch lengths------------- //
    write_branch_numbers(out_cache, T);

    //-------------------- Log model -------------------------//
    log_summary(out_cache, out_screen, out_both, full_imodels, full_smodels, full_scale_models, branch_length_model, P,args);

    //----------------- Tree-based constraints ----------------//
    if (args.count("t-constraint"))
	P.PC->TC = load_constraint_tree(args["t-constraint"].as<string>(), T.get_leaf_labels());

    if (args.count("a-constraint"))
	P.PC->AC = load_alignment_branch_constraints(args["a-constraint"].as<string>(),P.PC->TC);

    if (not extends(T, P.PC->TC))
	throw myexception()<<"Initial tree violates topology constraints.";

    //---------- Alignment constraint (horizontal) -----------//
    vector<string> ac_filenames(P.n_data_partitions(),"");
    if (args.count("align-constraint")) 
    {
	ac_filenames = split(args["align-constraint"].as<string>(),':');

	if (ac_filenames.size() != P.n_data_partitions())
	    throw myexception()<<"Need "<<P.n_data_partitions()<<" alignment constraints (possibly empty) separated by colons, but got "<<ac_filenames.size();
    }

    for(int i=0;i<P.n_data_partitions();i++)
	P.PC->DPC[i].alignment_constraint = load_alignment_constraint(ac_filenames[i],T);

    //------------------- Handle heating ---------------------//
    setup_heating(proc_id,args,P);

    return P;
}

void write_initial_alignments(const owned_ptr<Model>& M, int proc_id, string dir_name)
{
    const Parameters* P = M.as<const Parameters>();
    if (not P) return;

    if (P->t().n_nodes() < 2) return;

    vector<alignment> A;
    for(int i=0;i<P->n_data_partitions();i++)
	A.push_back((*P)[i].A());

    string base = dir_name + "/" + "C" + convertToString(proc_id+1);
    for(int i=0;i<A.size();i++)
    {
	checked_ofstream file(base+".P"+convertToString(i+1)+".initial.fasta");
	file<<A[i]<<endl;
    }
}

