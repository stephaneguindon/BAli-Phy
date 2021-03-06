SED = sed
GIT_VERSION_CMD = $(SHELL) $(top_srcdir)/scripts/git_version.sh
git_version.stamp:
	@if test -f "$(srcdir)/git_version.h"; then \
		if test -f "git_version.h"; then :; \
		else \
			cp "$(srcdir)/git_version.h" "git_version.h"; \
		fi; \
	fi
	$(GIT_VERSION_CMD) -k -s $(top_srcdir) -o git_version.h
	@if test -s "$(srcdir)/git_version.h"; then \
		if cmp "$(srcdir)/git_version.h" "git_version.h"; then :; \
		else \
			echo "Error: $(srcdir)/git_version.h and git_version.h differ."; \
			echo "       You probably want to remove the former."; \
			exit 1; \
		fi; \
	fi

dist-hook: git_version.stamp
	if test -f "git_version.h"; then \
		$(SED) -e 's|^#undef GIT_IS_DIST.*|#define GIT_IS_DIST 1|' \
			"git_version.h" > "$(distdir)/git_version.h"; \
	fi

SUBDIRS = . builtins/

#-----------------------------------------------------------------

noinst_HEADERS = dp/2way.H dp/dp-engine.H myexception.H tree/sequencetree.H dp/3way.H \
           dp/dp-matrix.H setup.H dp/5way.H math/eigenvalue.H \
           n_indels.H n_indels2.H alignment/alignment-constraint.H math/exponential.H \
           models/parameters.H substitution/cache.H alignment/alignment.H dp/hmm.H math/pow2.H \
           substitution/substitution.H dp/alignment-sums.H imodel/imodel.H probability/probability.H \
           alignment/alignment-util.H prior.H mcmc/proposals.H \
           tree/tree-branchnode.H sequence/alphabet.H math/log-double.H tree/tree.H \
           dp/bits.H math/logsum.H tree/tree-util.H probability/choose.H math/logprod.H \
           rng.H util.H object.H mcmc/mcmc.H mcmc/sample.H util-random.H \
           models/model.H sequence/sequence-format.H dp/dp-array.H monitor.H sequence/sequence.H \
           tools/bootstrap.H tools/inverse.H tools/statistics.H \
           tools/colors.H tools/joint-A-T.H  tools/stats-table.H \
           tools/distance-methods.H tools/optimize.H tools/tree-dist.H \
           tools/findroot.H tools/parsimony.H tools/parsimony2.H tools/mctree.H \
           version.H cow-ptr.H alignment/index-matrix.H cached_value.H \
	   tools/consensus-tree.H tools/partition.H mcmc/slice-sampling.H \
	   timer_stack.H mcmc/setup.H owned-ptr.H alignment/alignment-util2.H \
	   bounds.H io.H parser/parse.H models/TreeInterface.H \
           computation/computation.H computation/closure.H \
           computation/context.H computation/expression/AST_node.H \
	   computation/expression/expression_ref.H computation/expression/expression.H \
	   computation/expression/apply.H computation/expression/substitute.H \
	   computation/expression/constructor.H computation/model_expression.H \
	   computation/expression/indexify.H computation/expression/let.H \
	   computation/expression/case.H computation/expression/trim.H \
	   computation/expression/tuple.H computation/expression/list.H \
	   computation/bithash.H computation/graph_register.H computation/let-float.H \
           computation/loader.H computation/module.H computation/expression/dummy.H \
	   computation/operation.H computation/operations.H computation/operator.H \
           computation/pool.H computation/program.H computation/expression/lambda.H \
	   builtins/iota.H CacheList.H mcmc/logger.H models/setup.H \
           dp/dp_hmm.H parser/desugar.H mcmc/AIS.H tools/distance-report.H \
	   matrix.H builtins/vector_from_list.H startup/A-T-model.H startup/files.H startup/loggers.H \
	   startup/io.H startup/system.H startup/cmd_line.H computation/type_constant.H \
	   substitution/parsimony.H models/rules.H models/parse.H models/translate.H \
	   models/unification.H computation/optimization/simplifier.H tools/read-trees.H

LDFLAGS = @ldflags@ 

AM_CPPFLAGS = @AM_CPPFLAGS@ -I@top_srcdir@/src
AM_CXXFLAGS = @AM_CXXFLAGS@

LIBS = @libs@
if NO_SYSTEM_BOOST
LIBS += ../boost/lib/libboost_myparts.la
endif


#-----------------------------------------------------------------

TOOLS = model_P statreport alignment-gild \
	alignment-draw alignment-find \
	trees-to-SRQ tree-reroot alignment-chop-internal \
	alignment-convert alignment-thin \
	trees-consensus cut-range alignment-translate \
	alignment-median alignment-indices pickout \
	joint-indels alignment-consensus alignment-info alignment-smc\
	subsample tree-mean-lengths mctree-mean-lengths trees-bootstrap \
	joint-parsimony stats-merge stats-select alignment-identity \
	alignment-cat draw-graph \
	alignment-compare tree-partitions trees-distances \
	partitions-supported trees-pair-distances analyze-rates \
	path-graph alignment-find-conserved alignment-max alignments-diff \
	stats-cat

bin_PROGRAMS = bali-phy ${TOOLS}

if HAVE_CAIRO
bin_PROGRAMS += draw-tree	
endif

#-----------------------------------------------------------------

bali_phy_SOURCES = parser/parse.C dp/dp_hmm.C parser/desugar.C sequence/sequence.C \
          tree/tree.C alignment/alignment.C substitution/substitution.C mcmc/moves.C \
          rng.C math/exponential.C math/eigenvalue.C models/parameters.C prior.C mcmc/mcmc.C \
	  probability/choose.C tree/sequencetree.C mcmc/sample-branch-lengths.C \
	  util.C tree/randomtree.C sequence/alphabet.C bali-phy.C models/TreeInterface.C \
	  dp/hmm.C dp/dp-engine.C dp/dp-array.C dp/dp-matrix.C dp/3way.C dp/2way.C mcmc/sample-alignment.C \
	  mcmc/sample-node.C imodel/imodel.C dp/5way.C mcmc/sample-topology-NNI.C \
	  setup.C mcmc/sample-two-nodes.C sequence/sequence-format.C \
	  util-random.C alignment/alignment-random.C models/setup.C mcmc/sample-topology-SPR.C \
	  dp/alignment-sums.C alignment/alignment-util.C probability/probability.C models/model.C \
	  alignment/alignment-constraint.C monitor.C \
	  tree/tree-util.C myexception.C math/pow2.C substitution/parsimony.C \
	  tools/partition.C mcmc/proposals.C n_indels.C n_indels2.C alignment/alignment-util2.C \
	  tools/parsimony.C tools/parsimony2.C version.C mcmc/slice-sampling.C timer_stack.C \
	  mcmc/setup.C io.C mcmc/logger.C mcmc/AIS.C computation/operator.C \
	  computation/expression/expression.C computation/expression/constructor.C \
	  computation/expression/expression_ref.C computation/expression/AST_node.C \
	  computation/expression/apply.C computation/expression/substitute.C \
	  computation/expression/indexify.C computation/expression/let.C \
	  computation/expression/case.C  computation/expression/trim.C \
	  computation/expression/tuple.C computation/expression/list.C \
	  computation/expression/dummy.C computation/expression/lambda.C \
	  computation/computation.C computation/tokens.C computation/preprocess.C \
	  computation/graph_register.C computation/show_graph.C computation/module.C \
	  computation/model_expression.C computation/evaluate.C computation/gc.C computation/reroot.C \
	  computation/operations.C computation/loader.C computation/context.C computation/closure.C \
	  computation/let-float.C computation/program.C mcmc/sample-tri.C startup/A-T-model.C startup/files.C \
	  startup/loggers.C startup/system.C startup/cmd_line.C models/rules.C models/parse.C \
	  models/translate.C models/unification.C computation/optimization/simplifier.C \
	  tools/read-trees.C

if HOST_MINGW32
bali_phy_SOURCES += dlfcn-win32/dlfcn.c
endif

nodist_bali_phy_SOURCES = git_version.h

if HOST_LINUX
bali_phy_LDFLAGS = @ldflags@ -rdynamic
else
if HOST_WIN_ALL
bali_phy_LDFLAGS = @ldflags@ -Wl,--export-all-symbols,--out-implib=libbali-phy.dll.a
else
bali_phy_LDFLAGS = @ldflags@
endif
endif

if WITH_MPI
bali_phy_LDADD = ${LDADD} libsums.a @MPI_LDFLAGS@ 
if NO_SYSTEM_BOOST
bali_phy_LDADD += ../boost/lib/libboost_myparts_mpi.la
else
bali_phy_LDADD += ${LIBS} -lboost_mpi${BOOST_SUFFIX} -lboost_serialization${BOOST_SUFFIX}
endif
else
bali_phy_LDADD = ${LDADD} libsums.a
endif


# always "rebuild" these
BUILT_SOURCES = version.C git_version.stamp

noinst_LIBRARIES = libsums.a
libsums_a_SOURCES = computation/sums.C
libsums_a_CXXFLAGS = ${AM_CXXFLAGS} -fno-fast-math
libsums_a_CFLAGS = ${AM_CFLAGS} -fno-fast-math

#-----------------------------------------------------------------

model_P_SOURCES = tools/model_P.C tools/statistics.C rng.C util.C

#-------------------------- statreport --------------------------

statreport_SOURCES = tools/statreport.C tools/statistics.C util.C tools/stats-table.C io.C

#-------------------------- statreport --------------------------

stats_merge_SOURCES = tools/stats-merge.C util.C io.C

#-------------------------- statreport --------------------------

stats_select_SOURCES = tools/stats-select.C util.C tools/stats-table.C io.C

#-------------------------- statreport --------------------------

stats_cat_SOURCES = tools/stats-cat.C util.C tools/stats-table.C io.C

#-------------------------- statreport --------------------------

analyze_rates_SOURCES = tools/analyze-rates.C util.C tools/stats-table.C \
	tools/statistics.C io.C

#---------------------------------------------------------------

alignment_gild_SOURCES = tools/alignment-gild.C alignment/alignment.C sequence/alphabet.C \
	sequence/sequence.C util.C rng.C tree/tree.C tree/sequencetree.C tools/optimize.C \
	tools/findroot.C setup.C sequence/sequence-format.C \
	tools/distance-methods.C alignment/alignment-random.C alignment/alignment-util.C \
	tree/randomtree.C tree/tree-util.C tools/inverse.C io.C tools/read-trees.C

alignment_gild_LDADD = ${ATLAS_LIBS}

#---------------------------------------------------------------

alignment_median_SOURCES = tools/alignment-median.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C \
	tree/tree.C tree/sequencetree.C sequence/sequence-format.C alignment/alignment-util.C io.C tools/distance-report.C tools/statistics.C

#---------------------------------------------------------------

alignment_consensus_SOURCES = tools/alignment-consensus.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C \
	tree/tree.C tree/sequencetree.C util-random.C tools/statistics.C \
	sequence/sequence-format.C alignment/alignment-util.C alignment/index-matrix.C io.C

#---------------------------------------------------------------

alignment_max_SOURCES = tools/alignment-max.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C \
	tree/tree.C tree/sequencetree.C util-random.C tools/statistics.C \
	sequence/sequence-format.C alignment/alignment-util.C alignment/index-matrix.C io.C

#---------------------------------------------------------------

alignment_compare_SOURCES = tools/alignment-compare.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C \
	tree/tree.C tree/sequencetree.C util-random.C \
	sequence/sequence-format.C alignment/alignment-util.C io.C

#---------------------------------------------------------------

alignment_identity_SOURCES = tools/alignment-identity.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C \
	tree/tree.C tree/sequencetree.C util-random.C tools/statistics.C \
	sequence/sequence-format.C alignment/alignment-util.C alignment/index-matrix.C io.C

#---------------------------------------------------------------

alignment_thin_SOURCES = tools/alignment-thin.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C \
	tree/tree.C tree/sequencetree.C setup.C sequence/sequence-format.C tree/randomtree.C \
	alignment/alignment-util.C alignment/alignment-random.C tree/tree-util.C \
	tools/distance-methods.C tools/inverse.C alignment/index-matrix.C io.C tools/read-trees.C


#---------------------------------------------------------------

alignment_chop_internal_SOURCES = tools/alignment-chop-internal.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C tree/tree.C \
	sequence/sequence-format.C alignment/alignment-util.C io.C tree/sequencetree.C

#---------------------------------------------------------------

alignment_indices_SOURCES = tools/alignment-indices.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C tree/tree.C sequence/sequence-format.C alignment/alignment-util.C io.C tree/sequencetree.C

#---------------------------------------------------------------

alignments_diff_SOURCES = tools/alignments-diff.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C tree/tree.C sequence/sequence-format.C alignment/alignment-util.C io.C tree/sequencetree.C

#---------------------------------------------------------------

alignment_draw_SOURCES = tools/alignment-draw.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C sequence/sequence-format.C util.C alignment/alignment-util.C tools/colors.C tree/tree.C io.C tree/sequencetree.C

#---------------------------------------------------------------

joint_indels_SOURCES = tools/joint-indels.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C tree/tree.C tree/sequencetree.C tree/tree-util.C setup.C sequence/sequence-format.C alignment/alignment-random.C alignment/alignment-util.C tree/randomtree.C tools/statistics.C tools/joint-A-T.C tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------

joint_parsimony_SOURCES = tools/joint-parsimony.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C tree/tree.C \
	tree/sequencetree.C tree/tree-util.C setup.C sequence/sequence-format.C \
	alignment/alignment-random.C alignment/alignment-util.C tree/randomtree.C \
	tools/parsimony.C tools/joint-A-T.C n_indels.C io.C tools/read-trees.C

#---------------------------------------------------------------

alignment_info_SOURCES = tools/alignment-info.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C tree/tree.C tree/sequencetree.C setup.C tools/parsimony.C sequence/sequence-format.C tree/randomtree.C alignment/alignment-util.C alignment/alignment-random.C tree/tree-util.C tools/statistics.C io.C tools/read-trees.C

#---------------------------------------------------------------

alignment_smc_SOURCES = tools/alignment-smc.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C tree/tree.C tree/sequencetree.C setup.C tools/parsimony.C sequence/sequence-format.C tree/randomtree.C alignment/alignment-util.C alignment/alignment-random.C tree/tree-util.C tools/statistics.C io.C tools/read-trees.C

#---------------------------------------------------------------

alignment_cat_SOURCES = tools/alignment-cat.C sequence/alphabet.C sequence/sequence.C util.C sequence/sequence-format.C io.C \
	tree/tree.C tree/sequencetree.C tools/optimize.C tools/findroot.C 

#---------------------------------------------------------------

alignment_translate_SOURCES = tools/alignment-translate.C alignment/alignment.C alignment/alignment-util.C sequence/alphabet.C sequence/sequence.C sequence/sequence-format.C util.C tree/tree.C setup.C tree/sequencetree.C tree/randomtree.C rng.C tree/tree-util.C alignment/alignment-random.C io.C tools/tree-dist.C tools/partition.C tools/read-trees.C

#---------------------------------------------------------------

alignment_find_SOURCES = tools/alignment-find.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C alignment/alignment-util.C rng.C util.C sequence/sequence-format.C tree/tree.C io.C tree/sequencetree.C

#---------------------------------------------------------------

alignment_convert_SOURCES = tools/alignment-convert.C alignment/alignment.C alignment/alignment-util.C sequence/sequence.C sequence/alphabet.C util.C sequence/sequence-format.C tree/tree.C io.C tree/sequencetree.C

#---------------------------------------------------------------

alignment_find_conserved_SOURCES = tools/alignment-find-conserved.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C rng.C tree/tree.C tree/sequencetree.C setup.C tools/parsimony.C sequence/sequence-format.C tree/randomtree.C alignment/alignment-util.C alignment/alignment-random.C tree/tree-util.C tools/statistics.C tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------

trees_consensus_SOURCES = tools/trees-consensus.C tree/tree.C tree/sequencetree.C tools/tree-dist.C util.C tools/statistics.C tree/tree-util.C tools/mctree.C rng.C  tools/partition.C tools/consensus-tree.C io.C tools/read-trees.C

#---------------------------------------------------------------

trees_bootstrap_SOURCES = tools/trees-bootstrap.C tree/tree.C tree/sequencetree.C tools/tree-dist.C util.C rng.C tools/statistics.C tools/bootstrap.C tree/tree-util.C  tools/partition.C tools/consensus-tree.C io.C tools/read-trees.C

#---------------------------------------------------------------

partitions_supported_SOURCES = tools/partitions-supported.C tree/tree.C tree/sequencetree.C tools/tree-dist.C util.C  tools/statistics.C tree/tree-util.C  tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------

draw_graph_SOURCES = tools/draw-graph.C tree/tree.C tree/sequencetree.C tools/tree-dist.C util.C tree/tree-util.C tools/mctree.C rng.C  tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------
tree_mean_lengths_SOURCES = tools/tree-mean-lengths.C util.C tree/tree.C tree/sequencetree.C tools/tree-dist.C tools/statistics.C tree/tree-util.C  tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------
mctree_mean_lengths_SOURCES = tools/mctree-mean-lengths.C util.C tree/tree.C tree/sequencetree.C tools/tree-dist.C tools/statistics.C tree/tree-util.C tools/mctree.C rng.C tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------
trees_pair_distances_SOURCES = tools/trees-pair-distances.C util.C tree/tree.C tree/sequencetree.C tools/tree-dist.C tools/statistics.C tree/tree-util.C  tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------
tree_partitions_SOURCES = tools/tree-partitions.C util.C tree/tree.C tree/sequencetree.C tools/tree-dist.C tree/tree-util.C  tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------

trees_to_SRQ_SOURCES = tools/trees-to-SRQ.C util.C tree/tree.C tree/sequencetree.C tools/tree-dist.C tools/read-trees.C tools/statistics.C tree/tree-util.C tools/partition.C io.C

#---------------------------------------------------------------

tree_reroot_SOURCES = tools/tree-reroot.C tree/tree.C tree/sequencetree.C tree/tree-util.C util.C tools/tree-dist.C tools/partition.C io.C tools/read-trees.C

#---------------------------------------------------------------

pickout_SOURCES = tools/pickout.C util.C io.C

#---------------------------------------------------------------

subsample_SOURCES = tools/subsample.C io.C

#---------------------------------------------------------------

cut_range_SOURCES = tools/cut-range.C util.C io.C

#---------------------------------------------------------------

#analyze_distances_SOURCES = tools/analyze_distances.C alignment/alignment.C sequence/alphabet.C \
#	sequence/sequence.C tools/distance-methods.C \
#	util.C tree/sequencetree.C substitution/substitution.C eigenvalue.C tree/tree.C \
#	exponential.C models/setup.C models/smodel.C imodel.C rng.C prior.C \
#	choose.C tools/optimize.C setup.C models/rates.C alignment/alignment-util.C \
#	sequence/sequence-format.C tree/randomtree.C model.C computation/graph_register.C  probability.C \
#	substitution/substitution-cache.C substitution/substitution-index.C substitution/substitution-star.C tree/tree-util.C \
#	alignment-random.C parameters.C myexception.C monitor.C \
#	tools/tree-dist.C tools/inverse.C distribution.C tools/partition.C \
#	timer_stack.C io.C computation/operator.C computation/expression.C models/objects.C \
#	computation/computation.C computation/model_expression.C models/operations.C \
#	distribution-operations.C 2way.C computation/operations.C computation/module.C \
#	computation/loader.C computation/context.C

#---------------------------------------------------------------

trees_distances_SOURCES = tools/trees-distances.C tree/tree.C \
	tree/sequencetree.C tools/tree-dist.C tools/partition.C util.C \
	tree/tree-util.C tools/statistics.C io.C tools/distance-report.C tools/read-trees.C

#---------------------------------------------------------------

draw_tree_SOURCES = tools/draw-tree.C tree/tree.C tree/sequencetree.C \
	tools/tree-dist.C util.C tree/tree-util.C tools/mctree.C rng.C \
	util-random.C tools/partition.C io.C tools/read-trees.C
draw_tree_LDADD = ${CAIRO_LIBS}

#---------------------------------------------------------------

path_graph_SOURCES = tools/path-graph.C alignment/alignment.C sequence/alphabet.C sequence/sequence.C util.C \
	sequence/sequence-format.C alignment/alignment-util.C tree/tree.C io.C tree/sequencetree.C

#---------------------------------------------------------------
