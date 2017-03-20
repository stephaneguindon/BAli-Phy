#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "computation/expression/expression_ref.H"

struct simplifier_options
{
    bool pre_inline_unconditionally = true;
    bool post_inline_unconditionally = true;
    bool let_float_from_case = true;
    bool let_float_from_apply = true;
    bool let_float_from_let = true;
    bool case_of_constant = true;
    bool case_of_variable = true;
    bool beta_reduction = true;
};

expression_ref simplifier(const expression_ref& E1);

#endif