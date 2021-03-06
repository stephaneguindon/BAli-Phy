#include <set>
#include "lambda.H"
#include "dummy.H"
#include "case.H"
#include "let.H"
#include "substitute.H"
#include "expression.H"
#include "AST_node.H"
#include "computation/operations.H"

using std::vector;

/// R = case T of {patterns[i] -> bodies[i]}
bool parse_case_expression(const expression_ref& E, expression_ref& object, vector<expression_ref>& patterns, vector<expression_ref>& bodies)
{
    patterns.clear();
    bodies.clear();

    if (not is_case(E)) return false;

    object = E.sub()[0];

    for(auto& alt: E.sub()[1].sub())
    {
	patterns.push_back( alt.sub()[0] );
	bodies.push_back( alt.sub()[1] );
    }

    return true;
}

bool is_irrefutable_pattern(const expression_ref& E)
{
    return E.is_a<dummy>();
}

/// Is this either (a) irrefutable, (b) a constant, or (c) a constructor whose arguments are irrefutable patterns?
bool is_simple_pattern(const expression_ref& E)
{
    // (a) Is this irrefutable?
    if (is_irrefutable_pattern(E)) return true;

    // (b) Is this a constant with no arguments? (This can't be an irrefutable pattern, since we've already bailed on dummy variables.)
    if (not E.size()) return true;

    assert(E.head().is_a<constructor>());

    // Arguments of multi-arg constructors must all be irrefutable patterns
    for(int j=0;j<E.size();j++)
	if (not is_irrefutable_pattern(E.sub()[j]))
	    return false;

    // (c) Is this a constructor who arguments are irrefutable patterns?
    return true;
}


// This function currently assumes that all the patterns are just variables.

expression_ref case_expression(const expression_ref& T, const vector<expression_ref>& patterns, const vector<expression_ref>& bodies);

/// Create the expression case T of {patterns[i] -> bodies[i]} --- AND all patterns are just C[i] v1 v2 ... vn
expression_ref simple_case_expression(const expression_ref& T, const vector<expression_ref>& patterns, const vector<expression_ref>& bodies)
{
    expression_ref E = make_case_expression(T, patterns, bodies);

    for(int i=patterns.size()-1;i>=0;i--)
    {
	if (not is_simple_pattern(patterns[i]))
	    throw myexception()<<"simple_case_expression( ): pattern '"<<patterns[i]<<"' is not free variable in expression '"<<E<<"'";
    }

    return E;
}

expression_ref make_case_expression(const expression_ref& object, const vector<expression_ref>& patterns, const vector<expression_ref>& bodies)
{
    assert(patterns.size() == bodies.size());
    assert(not patterns.empty());

    expression_ref alts = AST_node("alts");
    for(int i=0;i<patterns.size();i++)
    {
	expression_ref alt = AST_node("alt");
	alt = alt + patterns[i] + bodies[i];
	alts = alts + alt;
    }

    expression_ref E = Case();
    E = E + object + alts;

    return E;
}

int find_object(const vector<expression_ref>& v, const expression_ref& E)
{
    for(int i=0;i<v.size();i++)
	if (E == v[i])
	    return i;
    return -1;
}

// FIXME: we perform 3 case operations in the case of zip x:xs [] because we create an 'otherwise' let-var that
//        performs a case on y:ys that has already been done.

/*
 * case (x[0],..,x[N-1]) of (p[0...M-1][0...N-1] -> b[0..M-1])
 *
 * 1. Categorize each rule according to the type of its top-level pattern.
 * 2. Substitute for the irrefutable rules to find the 'otherwise' branch.
 * 3. Find the bodies for what happens after we match the various constants.
 *
 * If the otherwise branch is used twice, then construct a let-expression for it.
 *
 */
expression_ref block_case(const vector<expression_ref>& x, const vector<vector<expression_ref>>& p, const vector<expression_ref>& b)
{
    const int N = x.size();
    const int M = p.size();

    assert(p.size() == b.size());

    // Each pattern must have N components.
    for(int j=0;j<M;j++)
	assert(p[j].size() == N);

    if (not x.size())
	return b[0];

    // 1. Categorize each rule according to the type of its top-level pattern
    vector<expression_ref> constants;
    vector< vector<int> > rules;
    vector<int> irrefutable_rules;
    for(int j=0;j<M;j++)
    {
	if (is_dummy(p[j][0]))
	{
	    irrefutable_rules.push_back(j);
	    continue;
	}

	expression_ref C = p[j][0].head();
	int which = find_object(constants, C);

	if (which == -1)
	{
	    which = constants.size();
	    constants.push_back(C);
	    rules.push_back(vector<int>{});
	}

	rules[which].push_back(j);
    }

    // 2. Substitute for the irrefutable rules to find the 'otherwise' branch
    // This is substitute(x[1],p[2..m][1], case x2...xN of p[2..M][i] -> b[2..M] )
    expression_ref otherwise;
    if (irrefutable_rules.empty())
	; // otherwise = NULL
    else
    {
	vector<expression_ref> x2 = x;
	x2.erase(x2.begin());

	vector<vector<expression_ref>> p2;
	vector<expression_ref> b2;
	for(int i=0;i<irrefutable_rules.size();i++)
	{
	    int r = irrefutable_rules[i];
	    p2.push_back(p[r]);
	    p2.back().erase(p2.back().begin());

	    b2.push_back(b[r]);

	    if (is_wildcard(p[r][0]))
		// This is a dummy.
		; //assert(d->name.size() == 0);
	    else
	    {
		// FIXME! What if x[0] isn't a var?
		// Then if *d occurs twice, then we should use a let expression, right?
		b2[i] = substitute(b2[i], p[r][0].as_<dummy>(), x[0]);
	    }
	}
      
	if (x2.empty())
	{
	    // If (b2.size() > 1) then we have duplicate irrefutable rules, but that's OK.
	    // This can even be generated in the process of simplifying block_case expressions.	
	    otherwise = b2[0];
	}
	else
	    otherwise = block_case(x2, p2, b2);
    }
      
    // If there are no conditions on x[0], then we are done.
    if (constants.empty())
    {
	assert(otherwise);
	return otherwise;
    }

    // Find the first safe var index
    std::set<dummy> free;

    for(int i=0;i<x.size();i++)
	add(free, get_free_indices(x[i]));

    for(int i=0;i<p.size();i++)
    {
	add(free, get_free_indices(b[i]));
  
	for(int j=0; j<p[i].size(); j++)
	    add(free, get_free_indices(p[i][j]));
    }
  
    int var_index = 0;
    if (not free.empty()) var_index = max_index(free)+1;

    // WHEN should we put the otherwise expression into a LET variable?
    expression_ref O;
    if (otherwise) O = dummy(var_index++);

    // 3. Find the modified bodies for the various constants
    vector<expression_ref> simple_patterns;
    vector<expression_ref> simple_bodies;
    bool all_simple_followed_by_irrefutable = true;

    for(int c=0;c<constants.size();c++)
    {
	// Find the arity of the constructor
	int arity = 0;
	if (constants[c].head().is_a<constructor>())
	    arity = constants[c].head().as_<constructor>().n_args();

	// Construct the simple pattern for constant C
	expression_ref H = constants[c];

	vector<expression_ref> S(arity);
	for(int j=0;j<arity;j++)
	    S[j] = dummy(var_index+j);

	int r0 = rules[c][0];

	simple_patterns.push_back({H,S});
	simple_bodies.push_back({});
    
	// Construct the objects for the sub-case expression: x2[i] = v1...v[arity], x[2]...x[N]
	vector<expression_ref> x2;
	for(int j=0;j<arity;j++)
	    x2.push_back(S[j]);
	x2.insert(x2.end(), x.begin()+1, x.end());

	// Are all refutable patterns on x[1] simple and followed by irrefutable patterns on x[2]...x[N]?
	bool future_patterns_all_irrefutable = true;

	// Construct the various modified bodies and patterns
	vector<expression_ref> b2;
	vector<vector<expression_ref> > p2;
	for(int i=0;i<rules[c].size();i++)
	{
	    int r = rules[c][i];

	    // Add the pattern
	    p2.push_back(vector<expression_ref>{});
	    assert(p[r][0].size() == arity);

	    // Add sub-patterns of p[r][1]
	    for(int k=0;k<arity;k++)
		p2.back().push_back(p[r][0].sub()[k]);

	    p2.back().insert(p2.back().end(), p[r].begin()+1, p[r].end());

	    // Add the body
	    b2.push_back(b[r]);

	    // Check if p2[i] are all irrefutable
	    for(int i=0;i<p2.back().size();i++)
		if (not is_irrefutable_pattern(p2.back()[i]))
		{
		    future_patterns_all_irrefutable = false;
		    all_simple_followed_by_irrefutable = false;
		}
	}

	// If x[1] matches a simple pattern in the only alternative, we may as well
	// not change the variable names for the match slots in this pattern.
	if (rules[c].size() == 1 and is_simple_pattern(p[r0][0]))
	{
	    simple_patterns.back() = p[r0][0];

	    // case x[1] of p[r0][1] -> case (x[2],..,x[N]) of (p[r0][2]....p[r0][N]) -> b[r0]
	    x2 = x;
	    x2.erase(x2.begin());

	    p2.back() = p[r0];
	    p2.back().erase( p2.back().begin() );
	}

	// If all future patterns are irrefutable, then we won't need to backtrack to the otherwise case.
	if (future_patterns_all_irrefutable)
	{
	    // There can be only one alternative.
	    assert(rules[c].size() == 1);

	    if (x2.size())
		simple_bodies.back() = block_case(x2, p2, b2);
	    else
		simple_bodies.back() = b[r0];
	}
	else
	{
	    if (otherwise)
	    {
		p2.push_back(vector<expression_ref>(x2.size(), dummy(-1)));
		// Since we could backtrack, use the dummy.  It will point to otherwise
		b2.push_back(O);
	    }
	    simple_bodies.back() = block_case(x2, p2, b2);
	}
    }

    if (otherwise)
    {
	simple_patterns.push_back(dummy(-1));
	// If we have any backtracking, then use the otherwise dummy, like the bodies.
	if (not all_simple_followed_by_irrefutable)
	    simple_bodies.push_back(O);
	else
	    simple_bodies.push_back(otherwise);
    }

    // Construct final case expression
    expression_ref CE = make_case_expression(x[0], simple_patterns, simple_bodies);

    if (otherwise and not all_simple_followed_by_irrefutable)
	CE = let_expression({{O.as_<dummy>(), otherwise}}, CE);

    return CE;
}

// Create the expression 'case T of {patterns[i] -> bodies[i]}'
// Create the expression 'case (T) of {(patterns[i]) -> bodies[i]}'
expression_ref case_expression(const expression_ref& T, const vector<expression_ref>& patterns, const vector<expression_ref>& bodies)
{
    vector<vector<expression_ref>> multi_patterns;
    for(const auto& p:patterns)
	multi_patterns.push_back({p});
    return block_case({T}, multi_patterns, bodies);
}

expression_ref case_expression(const expression_ref& T, const expression_ref& pattern, const expression_ref& body, const expression_ref& otherwise)
{
    vector<expression_ref> patterns = {pattern};
    vector<expression_ref> bodies = {body};
    if (otherwise and not pattern.is_a<dummy>())
    {
	patterns.push_back(dummy(-1));
	bodies.push_back(otherwise);
    }
    return case_expression(T,patterns, bodies);
}

expression_ref def_function(const vector< vector<expression_ref> >& patterns, const vector<expression_ref>& bodies)
{
    // Find the first safe var index
    std::set<dummy> free;

    for(int i=0;i<patterns.size();i++)
    {
	add(free, get_free_indices(bodies[i]));
  
	for(int j=0; j<patterns[i].size(); j++)
	    add(free, get_free_indices(patterns[i][j]));
    }
  
    int var_index = 0;
    if (not free.empty()) var_index = max_index(free)+1;

    // All versions of the function must have the same arity
    assert(patterns.size());
    for(int i=1;i<patterns.size();i++)
	assert(patterns[0].size() == patterns[i].size());

    // Construct the dummies
    vector<expression_ref> args;
    for(int i=0;i<patterns[0].size();i++)
	args.push_back(dummy(var_index+i));
    
    // Construct the case expression
    expression_ref E = block_case(args, patterns, bodies);

    // Turn it into a function
    for(int i=patterns[0].size()-1;i>=0;i--)
	E = lambda_quantify(var_index+i, E);

    return E;
}

bool is_case(const expression_ref& E)
{
    return E.head().type() == case_type;
}
