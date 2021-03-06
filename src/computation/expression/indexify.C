#include "indexify.H"
#include "expression.H"
#include "dummy.H"
#include "lambda.H"
#include "let.H"
#include "case.H"

using std::pair;
using std::vector;
using std::string;

template <typename T>
int find_index_backward(const vector<T>& v,const T& t)
{
    int L = v.size();
    for(int i=0;i<L;i++)
	if (v[L-i-1] == t)
	    return i;
    return -1;
}

expression_ref make_indexed_lambda(const expression_ref& R)
{
    return new expression(lambda2(),{R});
}

dummy get_named_dummy(int n)
{
    if (n<26) 
	return dummy(string{char(97+n)});
    else
	return dummy(n-26);
}

/// Convert to using de Bruijn indices.
expression_ref indexify(const expression_ref& E, const vector<dummy>& variables)
{
    if (not E.size())
    {
	// Indexed Variable - This is assumed to be a free variable, so just shift it.
	if (E.is_index_var())
	    return index_var(E.as_index_var() + variables.size());

	// Variable
	else if (E.is_a<dummy>())
	{
	    auto& D = E.as_<dummy>();
	    assert(not is_wildcard(E));

	    int index = find_index_backward(variables, D);
	    if (index == -1)
		throw myexception()<<"Dummy '"<<D<<"' is apparently not a bound variable in '"<<E<<"'?";
	    else
		return index_var(index);
	}
	// Constant
	else
	    return E;
    }
  
    // Lambda expression - /\x.e
    if (E.head().is_a<lambda>())
    {
	vector<dummy> variables2 = variables;
	variables2.push_back(E.sub()[0].as_<dummy>());
	return make_indexed_lambda( indexify(E.sub()[1], variables2) );
    }

    // Let expression
    vector<pair<dummy,expression_ref>> decls;
    vector<expression_ref> bodies;
    expression_ref T;
    if (parse_let_expression(E, decls, T))
    {
	vector<dummy> variables2 = variables;
	for(const auto& decl: decls)
	    variables2.push_back(decl.first);

	for(int i=0;i<decls.size();i++)
	    bodies.push_back( indexify(decls[i].second, variables2) );

	T = indexify(T, variables2);

	return indexed_let_expression(bodies,T);
    }

    // case expression
    vector<expression_ref> patterns;
    if (parse_case_expression(E, T, patterns, bodies))
    {
	T = indexify(T, variables);

	for(int i=0;i<bodies.size();i++)
	{
	    // Handle c[i] x[i][1..n] -> body[i]
	    expression_ref& P = patterns[i];
	    expression_ref& B = bodies[i];

	    vector<dummy> variables2 = variables;
	    for(int j=0;j<P.size();j++)
		variables2.push_back(P.sub()[j].as_<dummy>());

#ifndef NDEBUG
	    // FIXME - I guess this doesn't handle case a of b -> f(b)?
	    if (is_dummy(P))
		assert(is_wildcard(P));
#endif

	    P = P.head();
	    B = indexify(B, variables2);
	}

	return make_case_expression(T, patterns, bodies);
    }

    // If we've gotten this far, just transform the sub-expressions.
    // (This could be: Op, constructor, 
    expression* V = E.as_expression().clone();
    for(int i=0;i<V->size();i++)
	V->sub[i] = indexify(V->sub[i], variables);
    return V;
}

expression_ref indexify(const expression_ref& E)
{
    return indexify(E,vector<dummy>{});
}

/// Convert to using de Bruijn indices.
expression_ref deindexify(const expression_ref& E, const vector<expression_ref>& variables)
{
    if (not E.size())
    {
	// Indexed Variable - This is assumed to be a free variable, so just shift it.
	if (E.is_index_var())
	{
	    int index = E.as_index_var();
	    if (index >= variables.size())
		return index_var(index - variables.size());

	    return variables[variables.size()-1 - index];
	}
	// Constant
	else
	    return E;
    }
  
    // Lambda expression - /\x.e
    if (E.head().is_a<lambda2>())
    {
	vector<expression_ref> variables2 = variables;
	dummy d = get_named_dummy(variables.size());
	variables2.push_back(d);
	return lambda_quantify(d,deindexify(E.sub()[0],variables2));
    }

    // Let expression
    vector<expression_ref> bodies;
    vector<pair<dummy,expression_ref>> decls;
    expression_ref body;
    if (parse_indexed_let_expression(E, bodies, body))
    {
	vector<expression_ref> variables2 = variables;
	for(int i=0;i<bodies.size();i++)
	{
	    dummy d = get_named_dummy(variables2.size());
	    decls.push_back({d, bodies[i]});
	    variables2.push_back( d );
	}

	// Deindexify let-bound stmts only after list of variables has been extended.
	for(int i=0;i<decls.size();i++)
	    decls[i].second = deindexify(decls[i].second, variables2);

	body = deindexify(body, variables2);

	return let_expression(decls, body);
    }

    // case expression
    vector<expression_ref> patterns;
    expression_ref T;
    if (parse_case_expression(E, T, patterns, bodies))
    {
	T = deindexify(T, variables);

	for(int i=0;i<bodies.size();i++)
	{
	    assert(not patterns[i].size());
	    // Make a new expression so we can add variables to the pattern if its a constructor
	    expression_ref P = patterns[i];
	    expression_ref& B = bodies[i];

	    // Find the number of arguments in the constructor
	    int n_args = 0;
	    if (P.head().is_a<constructor>())
		n_args = P.head().as_<constructor>().n_args();

	    // Add n_arg variables to the stack and to the pattern
	    vector<expression_ref> variables2 = variables;
	    for(int j=0;j<n_args;j++)
	    {
		dummy d = get_named_dummy(variables2.size());
		variables2.push_back( d );
		P = P + d;
	    }

#ifndef NDEBUG
	    if(is_dummy(P))
		assert(is_wildcard(P));
#endif

	    patterns[i] = P;
	    B = deindexify(B, variables2);
	}

	return make_case_expression(T, patterns, bodies);
    }

    // If we've gotten this far, just transform the sub-expressions.
    // (This could be: Op, constructor, 
    expression* V = E.as_expression().clone();
    for(int i=0;i<V->size();i++)
	V->sub[i] = deindexify(V->sub[i], variables);
    return V;
}

expression_ref deindexify(const expression_ref& E)
{
    return deindexify(E,vector<expression_ref>{});
}

