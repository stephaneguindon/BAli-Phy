/*
  Copyright (C) 2004-2010 Benjamin Redelings

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_FENV_H
extern "C" {
#include "fenv.h"
}
#endif

#include "timer_stack.H"

#ifdef HAVE_MPI
#include <mpi.h>
#include <boost/mpi.hpp>
namespace mpi = boost::mpi;
#endif

#include <cmath>
#include <ctime>
#include <iostream>
#include <sstream>
#include <new>
#include <map>

#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/chrono.hpp>

#include "substitution/substitution.H"
#include "myexception.H"
#include "rng.H"
#include "models/parameters.H"
#include "mcmc/mcmc.H"
#include "util.H"
#include "setup.H"
#include "monitor.H"
#include "math/pow2.H"
#include "version.H"
#include "mcmc/setup.H"
#include "io.H"
#include "parser/desugar.H"
#include "computation/module.H"
#include "computation/program.H"

#include "startup/A-T-model.H"
#include "startup/files.H"
#include "startup/loggers.H"
#include "startup/io.H"
#include "startup/system.H"
#include "startup/cmd_line.H"
#include "computation/expression/expression.H"
#include "computation/loader.H"

namespace fs = boost::filesystem;
namespace chrono = boost::chrono;

namespace po = boost::program_options;
using po::variables_map;

using std::cout;
using std::cerr;
using std::clog;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;
using std::map;
using std::set;

using std::shared_ptr;

#ifdef DEBUG_MEMORY
void * operator new(size_t sz)
    printf("new called, sz = %d\n",sz);
    return malloc(sz); 
}

void operator delete(void * p)
    printf("delete called, content = %d\n",(*(int*)p));
    free(p); 
}
#endif

/*
  int parameter_with_extension(const Model& M, const string& name)
  {
  vector<int> indices = parameters_with_extension(M, name);
  if (not indices.size())
  return -1;

  if (indices.size() == 1)
  return indices.back();

  myexception e;
  e<<"Multiple parameter names fit the pattern '"<<name<<"':\n";
  for(int i=0;i<indices.size();i++)
  e<<"   * "<<M.parameter_name(indices[i])<<"\n";

  throw e;
  }
*/

// How to record that the user said e.g. "fix the alignment"?  Or, fix parameter X?  Should we?

void set_key_values(Model& M, const variables_map& args)
{
    if (not args.count("set")) return;

    vector<string> key_value_pairs = args["set"].as<vector<string> >();

    for(const auto& key_value_pair: key_value_pairs)
    {
	vector<string> parse = split(key_value_pair,'=');
	if (parse.size() != 2)
	    throw myexception()<<"Ill-formed key-value pair '"<<key_value_pair<<"'.";

	string key = parse[0];

	double value = convertTo<double>(parse[1]);
    
	(*M.keys.modify())[key] = value;
    }
}

/// Parse command line arguments of the form --fix X=x or --unfix X=x or --set X=x and modify P
void set_initial_parameter_values(Model& M, const variables_map& args) 
{
    //-------------- Specify fixed parameters ----------------//
    vector<string> doset;
    if (args.count("initial-value"))
	doset = args["initial-value"].as<vector<string> >();

    vector<string> short_names = short_parameter_names(M);

    // set parameters
    for(const auto& arg: doset)
    {
	//parse
	vector<string> parse = split(arg,'=');
	if (parse.size() != 2)
	    throw myexception()<<"Ill-formed initial condition '"<<arg<<"'.";

	string name = parse[0];
	expression_ref value;
	try {
	    value = parse_object(parse[1]);
	}
	catch (myexception& e)
	{
	    std::ostringstream o;
	    o<<"Setting parameter '"<<name<<"': ";
	    e.prepend(o.str());
	    throw e;
	}

	int p_index = M.find_parameter(name);
	if (p_index == -1)
	    p_index = find_index(short_names,name);
	if (p_index == -1)
	    throw myexception()<<"Can't find parameter '"<<name<<"' to set value '"<<parse[1]<<"'";

	M.set_parameter_value(p_index,value);
    }
}

/// Initialize the default random number generator and return the seed
unsigned long init_rng_and_get_seed(const variables_map& args)
{
    unsigned long seed = 0;
    if (args.count("seed")) {
	seed = args["seed"].as<unsigned long>();
	myrand_init(seed);
    }
    else
	seed = myrand_init();

    return seed;
}

chrono::system_clock::time_point start_time = chrono::system_clock::now();

string ctime(const chrono::system_clock::time_point& t)
{
    time_t t2 = chrono::system_clock::to_time_t(t);
    char* c = ctime(&t2);
    return c;
}

void show_ending_messages(bool show_only)
{
    using namespace chrono;

    if (not show_only or log_verbose) {

	extern long total_reductions;
	extern long total_changeable_eval;
	extern long total_changeable_eval_with_result;
	extern long total_changeable_eval_with_call;
	extern long total_changeable_reductions;
	extern long total_reg_allocations;
	extern long total_comp_allocations;
	extern long total_step_allocations;
	extern long total_destroy_token;
	extern long total_release_knuckle;
	extern long total_create_context1;
	extern long total_create_context2;
	extern long total_tokens;
	extern long max_version;
	extern long total_reroot;
	extern long total_reroot_one;
	extern long total_set_reg_value;
	extern long total_get_reg_value;
	extern long total_get_reg_value_non_const;
	extern long total_get_reg_value_non_const_with_result;
	extern long total_invalidate;
	extern long total_steps_invalidated;
	extern long total_results_invalidated;
	extern long total_steps_scanned;
	extern long total_results_scanned;
	extern long total_steps_pivoted;
	extern long total_results_pivoted;
	extern long total_context_pr;
	extern long total_gc;
	extern long total_regs;
	extern long total_steps;
	extern long total_comps;
  
  
	if (total_reductions > 0)
	{
	    cout<<"total changeable evals         = "<<total_changeable_eval<<endl;
	    cout<<"  with result                  = "<<total_changeable_eval_with_result<<endl;
	    cout<<"  with call but not result     = "<<total_changeable_eval_with_call<<endl;
	    cout<<"total reduction steps          = "<<total_reductions<<endl;
	    cout<<"  changeable reduction steps   = "<<total_changeable_reductions<<endl;
	    cout<<"  unchangeable reduction steps = "<<total_reductions-total_changeable_reductions<<endl;
	    cout<<"\ntotal garbage collection runs  = "<<total_gc<<endl;
	    cout<<"total register allocations     = "<<total_reg_allocations<<endl;
	    cout<<"total computation allocations  = "<<total_comp_allocations<<endl;
	    cout<<"total step allocations         = "<<total_step_allocations<<endl;
	    cout<<"total regs                     = "<<total_regs<<endl;
	    cout<<"total steps                    = "<<total_steps<<endl;
	    cout<<"total computations             = "<<total_comps<<endl;
	    cout<<"\ntotal reroot operations        = "<<total_reroot<<endl;
	    cout<<"  total atomic reroot          = "<<total_reroot_one<<endl;
	    cout<<"  total steps pivoted          = "<<total_steps_pivoted<<endl;
	    cout<<"  total results pivoted        = "<<total_results_pivoted<<endl;
	    cout<<"total invalidations            = "<<total_invalidate<<endl;
	    cout<<"  total steps invalidated      = "<<total_steps_invalidated<<endl;
	    cout<<"  total results invalidated    = "<<total_results_invalidated<<endl;
	    cout<<"  total steps scanned          = "<<total_steps_scanned<<endl;
	    cout<<"  total results scanned        = "<<total_results_scanned<<endl;
	    cout<<"total tokens                   = "<<total_tokens<<endl;
	    cout<<"maximum version                = "<<max_version<<endl;
	    cout<<"total tokens destroyed         = "<<total_destroy_token<<endl;
	    cout<<"  total knuckles destroyed     = "<<total_release_knuckle<<endl;
	    cout<<"total create context           = "<<total_create_context1+total_create_context2<<endl;
	    cout<<"  operator=                    = "<<total_create_context1<<endl;
	    cout<<"  copy constructor             = "<<total_create_context2<<endl;
	    cout<<"\ntotal values set               = "<<total_set_reg_value<<endl;
	    cout<<"total values gotten            = "<<total_get_reg_value<<endl;
	    cout<<"total values gotten variable   = "<<total_get_reg_value_non_const<<endl;
	    cout<<"total values gotten w/ result  = "<<total_get_reg_value_non_const_with_result<<endl;
	    cout<<"total context probability      = "<<total_context_pr<<endl;
	}
    }
    system_clock::time_point end_time = system_clock::now();
  
    if (end_time - start_time > seconds(2)) 
    {
	cout<<endl;
	cout<<"start time: "<<ctime(start_time)<<endl;
	cout<<"  end time: "<<ctime(end_time)<<endl;
	cout<<"total (elapsed) time: "<<duration_string( duration_cast<seconds>(end_time-start_time) )<<endl;
	cout<<"total (CPU) time: "<<duration_string( duration_cast<seconds>(total_cpu_time()) )<<endl;
    }
    if (substitution::total_calc_root_prob > 1 and not show_only) {
	cout<<endl;
	cout<<"total likelihood evals = "<<substitution::total_likelihood<<endl;
	cout<<"total calc_root_prob evals = "<<substitution::total_calc_root_prob<<endl;
	cout<<"total branches peeled = "<<substitution::total_peel_leaf_branches+substitution::total_peel_internal_branches<<endl;
	cout<<endl;
    }
}

/* 
 * 1. Add a PRANK-like initial algorithm.
 * 2. Add some kind of constraint.
 * 3. Improve the method for proposing new SPR attachment sites.
 *  3a. Can we walk along the tree making characters present?
 */

fs::path get_system_lib_path(const string& exe_name)
{
    fs::path system_lib_path = find_exe_path(exe_name);
    if (not system_lib_path.empty())
    {
	system_lib_path.remove_filename();
	system_lib_path = system_lib_path / "lib" / "bali-phy";

	if (not fs::exists(system_lib_path))
	    system_lib_path = "";
    }
    return system_lib_path;
}

fs::path get_user_lib_path()
{
    fs::path user_lib_path;
    if (getenv("HOME"))
    {
	user_lib_path = getenv("HOME");
	user_lib_path = user_lib_path / ".local" / "share" / "bali-phy" / "packages";

	if (not fs::exists(user_lib_path))
	    user_lib_path = "";
    }
    return user_lib_path;
}

std::shared_ptr<module_loader> setup_module_loader(variables_map& args, const string& filename)
{
    fs::path system_lib_path = get_system_lib_path(filename);
    fs::path user_lib_path = get_user_lib_path();

    module_loader L;

    // 1. Add user-specified package paths
    if (args.count("package-path"))
	for(const string& p: split(args["package-path"].as<string>(),':'))
	{
	    fs::path path = p;
	    L.try_add_plugin_path( (path / "modules").string() );
	    L.try_add_plugin_path( path.string() );
	}

    // 2. Add default user path
    if (not user_lib_path.empty())
    {
	fs::path user_module_path = user_lib_path;
	if (fs::exists(user_module_path))
	    L.plugins_path.push_back( user_module_path.string() );
    }
  
    // 3. Add default system paths
    if (not system_lib_path.empty())
    {
	L.try_add_plugin_path( (system_lib_path / "modules").string() );
	L.try_add_plugin_path( system_lib_path.string() );
    }

    // 4. Write out paths to C1.err
    if (log_verbose)
    {
	std::cerr<<"\nPlugins path = \n";
	for(const auto& path: L.plugins_path)
	    cerr<<"  "<<path<<"\n";
    }

    // 5a. Check for empty paths
    if (L.plugins_path.empty())
	throw myexception()<<"No plugin paths are specified!.  Use --package-path=<path> to specify the directory containing 'Prelude.so'.";

    // 5b. Check for Prelude.so
    try
    {
	L.find_plugin("Prelude");
    }
    catch (...)
    {
	throw myexception()<<"Can't find Prelude plugin.  Use --package-path=<path> to specify the directory containing 'Prelude"<<plugin_extension<<"'.";
    }

    // 5c. Check for Prelude.hs
    try
    {
	L.find_module("Prelude");
    }
    catch (...)
    {
	throw myexception()<<"Can't find Prelude in module path.  Use --package-path=<path> to specify the directory containing 'modules/Prelude.hs'.";
    }

    return std::shared_ptr<module_loader>(new module_loader(L));
}

int simple_size(const expression_ref& E);

int main(int argc,char* argv[])
{ 
    int n_procs = 1;
    int proc_id = 0;

#ifdef HAVE_MPI
    mpi::environment env(argc, argv);
    mpi::communicator world;

    proc_id = world.rank();
    n_procs = world.size();
#endif

    restore restore_cout(cout);
    restore restore_cerr(cerr);
    restore restore_clog(clog);

    std::ios::sync_with_stdio(false);

    ostream out_screen(cout.rdbuf());
    ostream err_screen(cerr.rdbuf());

    std::ostringstream out_cache;
    std::ostringstream err_cache;

    vector<shared_ptr<ostream>> files;

    teebuf tee_out(out_screen.rdbuf(), out_cache.rdbuf());
    teebuf tee_err(err_screen.rdbuf(), err_cache.rdbuf());

    ostream out_both(&tee_out);
    ostream err_both(&tee_err);

    int retval=0;

    bool show_only = false;

    try {

#if defined(HAVE_FEENABLEEXCEPT) && !defined(NDEBUG)
	feenableexcept(FE_DIVBYZERO|FE_OVERFLOW|FE_INVALID);
	//    feclearexcept(FE_DIVBYZERO|FE_OVERFLOW|FE_INVALID);
#endif
#if defined(HAVE_CLEAREXCEPT) && defined(NDEBUG)
	feclearexcept(FE_DIVBYZERO|FE_OVERFLOW|FE_INVALID);
#endif
	fp_scale::initialize();

	//---------- Parse command line  ---------//
	variables_map args = parse_cmd_line(argc,argv);

	show_only = args.count("test");

	//------ Increase precision for (cout,cerr) if we are testing ------//
	if (show_only)
	{
	    cerr.precision(15);
	    cout.precision(15);
	}

	//------ Capture copy of 'cerr' output in 'err_cache' ------//
	if (not show_only)
	    cerr.rdbuf(err_both.rdbuf());

	//------------- Setup module loader -------------//
	auto L = setup_module_loader(args, argv[0]);
	L->pre_inline_unconditionally = args["pre-inline"].as<bool>();
	L->post_inline_unconditionally = args["post-inline"].as<bool>();
	L->let_float_from_case = args["let-float-from-case"].as<bool>();
	L->let_float_from_apply = args["let-float-from-apply"].as<bool>();
	L->let_float_from_let = args["let-float-from-let"].as<bool>();
	L->case_of_constant = args["case-of-constant"].as<bool>();
	L->case_of_variable = args["case-of-variable"].as<bool>();
	L->beta_reduction = args["beta-reduction"].as<bool>();
	L->max_iterations = args["simplifier-max-iterations"].as<int>();

	//---------- Initialize random seed -----------//
	unsigned long seed = init_rng_and_get_seed(args);
    
	out_cache<<"random seed = "<<seed<<endl<<endl;

	//---------- test optimizer ----------------
	if (args.count("test-module"))
	{
	    string filename = args["test-module"].as<string>();
	    Module M ( L->read_module_from_file(filename) );

	    Program P(L);
	    P.add(M);
	    auto& M2 = P.get_module(M.name);
	    for(const auto& s: M2.get_symbols())
	    {
		const auto& S = s.second;
		if (S.body and S.scope == local_scope)
		    std::cerr<<"size = "<<simple_size(S.body)<<"   "<<S.name<<" = "<<S.body<<std::endl;
	    }
	    exit(0);
	}

	//---------- Create model object -----------//
	owned_ptr<Model> M;
	if (args.count("align"))
	    M = create_A_and_T_model(args, L, out_cache, out_screen, out_both, proc_id);
	else
	    M = Model(L);
	M->set_args(trailing_args(argc, argv, trailing_args_separator));

	//------------- Parse the Hierarchical Model description -----------//
	if (args.count("model"))
	{
	    const string filename = args["model"].as<string>();
	    read_add_model(*M,filename);
	}
	else if (args.count("Model"))
	{
	    const string filename = args["Model"].as<string>();
	    add_model(*M,filename);
	}
      
	if (args.count("tree") and M.as<Parameters>())
	{
	    auto P = M.as<Parameters>();
	    for(int i=0;i<P->n_branch_scales();i++)
		P->branch_scale(i, 1.0);
	}

	set_initial_parameter_values(*M,args);

	set_key_values(*M,args);

	//---------------Do something------------------//
	if (show_only)
	{
	    print_stats(cout,*M, log_verbose);

	    M->show_graph();
	}
	else 
	{
	    raise_cpu_limit(err_both);

	    block_signals();

	    long int max_iterations = args["iterations"].as<long int>();
	    int subsample = args["subsample"].as<int>();
      
	    //---------- Open output files -----------//
	    vector<MCMC::Logger> loggers;

	    string dir_name="";
	    if (not args.count("test")) {
#ifdef HAVE_MPI
		if (not proc_id) {
		    dir_name = init_dir(args);
	  
		    for(int dest=1;dest<n_procs;dest++) 
			world.send(dest, 0, dir_name);
		}
		else
		    world.recv(0, 0, dir_name);

		// cerr<<"Proc "<<proc_id<<": dirname = "<<dir_name<<endl;
#else
		dir_name = init_dir(args);
#endif
		files = init_files(proc_id, dir_name, argc, argv);
		{
		    vector<string> Rao_Blackwellize;
		    if (args.count("Rao-Blackwellize"))
			Rao_Blackwellize = split(args["Rao-Blackwellize"].as<string>(),',');
		    loggers = construct_loggers(M, subsample, Rao_Blackwellize, proc_id, dir_name);
		}
		write_initial_alignments(*M, proc_id, dir_name);
	    }
	    else {
		files.push_back(shared_ptr<ostream>(new ostream(cout.rdbuf())));
		files.push_back(shared_ptr<ostream>(new ostream(cerr.rdbuf())));
	    }

	    //------ Redirect output to files -------//
	    *files[0]<<out_cache.str(); out_cache.str("");
	    *files[1]<<err_cache.str(); err_cache.str("");

	    tee_out.setbuf2(files[0]->rdbuf());
	    tee_err.setbuf2(files[1]->rdbuf());

	    cout.flush() ; cout.rdbuf(files[0]->rdbuf());
	    cerr.flush() ; cerr.rdbuf(files[1]->rdbuf());
	    clog.flush() ; clog.rdbuf(files[1]->rdbuf());

	    //------ Redirect output to files -------//

	    // Force the creation of parameters
	    for(int i=0;i<M->n_parameters();i++)
		M->parameter_is_modifiable(i);

	    avoid_zero_likelihood(M, *files[0], out_both);

	    do_pre_burnin(args, M, *files[0], out_both);

	    out_screen<<"\nBeginning "<<max_iterations<<" iterations of MCMC computations."<<endl;
	    out_screen<<"\nBAli-Phy does NOT decide when to stop -- you need to decide yourself!"<<endl;
	    out_screen<<"   - Future screen output sent to '"<<dir_name<<"/C1.out'"<<endl;
	    out_screen<<"   - Future debugging output sent to '"<<dir_name<<"/C1.err'"<<endl;
	    if (M.as<Parameters>())
	    {
		out_screen<<"   - Sampled trees logged to '"<<dir_name<<"/C1.trees'"<<endl;
		out_screen<<"   - Sampled alignments logged to '"<<dir_name<<"/C1.P<partition>.fastas'"<<endl;
	    }
	    out_screen<<"   - Sampled numerical parameters logged to '"<<dir_name<<"/C1.p'"<<endl;
	    out_screen<<endl;
	    out_screen<<"You can examine 'C1.p' using BAli-Phy tool statreport (command-line) or the BEAST program Tracer (graphical).\n"<<endl;
	    out_screen<<"See the manual at http://www.bali-phy.org/README.xhtml for further information."<<endl;

	    //-------- Start the MCMC  -----------//
	    do_sampling(args, M, max_iterations, *files[0], loggers);

	    // Close all the streams, and write a notification that we finished all the iterations.
	    // close_files(files);
	}
    }
    catch (std::bad_alloc&) 
    {
	// 1. If we haven't yet moved screen output to a file, then write cached screen output.
	out_screen<<out_cache.str(); out_screen.flush();
	err_screen<<err_cache.str(); err_screen.flush();

	// 2. Now, write message to either (screen+cache) or (screen+file), and flush.
	err_both<<"Doh!  Some kind of memory problem?\n"<<endl;
	// 3. Write memory report to either (screen) or (screen+cache) or (screen+file)
	report_mem();
	retval=2;
    }
    catch (std::exception& e) 
    {
	// 1. If we haven't yet moved screen output to a file, then write cached screen output.
	out_screen<<out_cache.str(); out_screen.flush();
	err_screen<<err_cache.str(); err_screen.flush();

	// 2. Now, write message to either (screen+cache) or (screen+file), and flush.
	if (n_procs > 1)
	    err_both<<"bali-phy: Error["<<proc_id<<"]! "<<e.what()<<endl;
	else
	    err_both<<"bali-phy: Error! "<<e.what()<<endl;

	retval=1;
    }

    show_ending_messages(show_only);

    out_both.flush();
    err_both.flush();
    return retval;
}

///
/// \file bali-phy.C
///
/// \brief This file contains routines to parse and check input and initiate
///        the bali-phy program.
///

/// \mainpage
/// \section main Main
/// The program is initiated from the file bali-phy.C
///
/// \section subst Substitution
/// The substitution likelihood is calculated in the file
/// substitution.C.  In order to speed up likelihood calculations, we
/// cache conditional likelihoods (substitution-cache.C).  However,
/// because the alignment is changing, column numbers may change and
/// cannot be relied on.  Therefore, we compute indices for a branch
/// that do not change if the alignment of the subtree behind the
/// branch does not change (substitution-index.C).  Exponentials are
/// computed in exponential.C .
/// 
/// \section mcmc MCMC
/// The Markov Chain Monte Carlo (MCMC) routines are all called from
/// the file mcmc.C.  Markov chains are constructed in mcmc/setup.C.
/// Proposals for Metropolis-Hastings moves are defined in
/// proposals.C . Slice sampling routines are defined in
/// slice-sampling.C. 
///
/// \section objects Objects
/// Trees are defined in tree.C and tree-branchnode.H. Leaf-labelled
/// trees are defined in sequencetree.C.  Sequences are defined in
/// sequence.C. The types of sequence are defined in alphabet.C, which
/// contains classes that relate letters like A, T, G, and C to
/// integers.  Alignments are defined in alignment.C and place 
/// collection of sequences in a matrix to depict homology.  Methods
/// for reading FASTA and PHYLIP files are defined in
/// sequence-format.C.
///
/// \section models Models
/// In BAli-Phy, models are built from class Model.  Model objects
///   - depend on some number of parameters (all of type double)
///   - implement prior distributions on their parameters.
///
/// A Model object that implements the SuperModel interface can
/// contain other Model objects as parts - child Model parameters are
/// mapped to parameters in the parent Model.  However, two child
/// Models cannot (easily) share a parameter, because each Model
/// manages and 'owns' its own parameters.  This ownership 
/// means that a Model specifies:
/// - a prior distribution on its parameter vector
/// - a name (a string) for each parameter
/// - an attribute (a boolean) that determinies whether each parameter is fixed or variable
///
/// \section DP Dynamic Programming
/// Many of the sampling routines rely on dynamic programming.
/// Dynamic programming in turn relies on Hidden Markov Models
/// (HMMs).  A general HMM class is defined in hmm.C.  Specific HMMs
/// are define in 2way.C (pairwise alignments), 3way.C (three adjacent
/// pairwise alignments) and 5way.C (pairwise alignments on a branch
/// and its 4 adjacent branches).  Objects for performing 1-D dynamic
/// programming are defined in dp-array.C.  Objects for performing 2-D 
/// dynamic programming are defined in dp-matrix.C. 
///
/// There are a number of dynamic programming problems, and each of
/// them involves both (a) summing over all the alignments in some
/// set, and (b) sampling from the posterior distribution of
/// alignments in that set.  Each problem is distinguished by the
/// different set of alignments.  The file sample-alignment.C
/// implements summing/sampling the alignment on one branch (2D) using
/// the HMM in 2way.C.  The file sample-node.C implements resampling
/// the +/- state in each column for a specific internal node
/// sequence (1D) using the HMM in 3way.C. The file sample-tri.C
/// implements resampling the alignment along a branch, and also
/// resampling the +/- state in each column for a sequence at an
/// internal node on one end of the branch (2D) using the HMM in
/// 3way.C.  The file sample-two-nodes.C implements resample the +/-
/// state for sequence at two internal nodes connected by a branch
/// (1D) using the HMM in 5way.C.  The interface for these routes and
/// other related utility routines is in alignment-sums.H.
/// 
/// \section intro_sec Introduction
///
/// BAli-Phy is an MCMC sampler to jointly estimate alignments and a phylogeny.
///
///  

