#include "computation.H"
#include "expression.H"
#include "graph_register.H"

int OperationArgs::reg_for_slot(int slot) const
{
    int index = reference(slot).as_index_var();

    return current_closure().lookup_in_env(index);
}

int OperationArgs::n_args() const {return current_closure().exp.size();}

const expression_ref& OperationArgs::reference(int slot) const {return current_closure().exp.sub()[slot];}

const closure& OperationArgs::evaluate_slot_to_closure(int slot)
{
    return evaluate_reg_to_closure(reg_for_slot(slot));
}

const closure& OperationArgs::evaluate_slot_to_closure_(int slot)
{
    return evaluate_reg_to_closure_(reg_for_slot(slot));
}

int OperationArgs::evaluate_slot_no_record(int slot)
{
    return evaluate_reg_no_record(reg_for_slot(slot));
}

int OperationArgs::evaluate_slot_to_reg(int slot)
{
    return evaluate_reg_to_reg(reg_for_slot(slot));
}

const expression_ref& OperationArgs::evaluate_reg_to_object(int R2)
{
    const expression_ref& result = evaluate_reg_to_closure(R2).exp;
#ifndef NDEBUG
    if (result.head().is_a<lambda2>())
	throw myexception()<<"Evaluating lambda as object: "<<result.print();
#endif
    return result;
}

const expression_ref& OperationArgs::evaluate_reg_to_object_(int R2)
{
    const expression_ref& result = evaluate_reg_to_closure_(R2).exp;
#ifndef NDEBUG
    if (result.head().is_a<lambda2>())
	throw myexception()<<"Evaluating lambda as object: "<<result.print();
#endif
    return result;
}

const expression_ref& OperationArgs::evaluate_slot_to_object(int slot)
{
    return evaluate_reg_to_object(reg_for_slot(slot));
}

const expression_ref& OperationArgs::evaluate_slot_to_object_(int slot)
{
    return evaluate_reg_to_object_(reg_for_slot(slot));
}

const expression_ref& OperationArgs::evaluate(int slot)
{
    return evaluate_slot_to_object(slot);
}

const expression_ref& OperationArgs::evaluate_(int slot)
{
    return evaluate_slot_to_object_(slot);
}


void OperationArgs::evaluate_to_constant(const closure& C)
{
    if (C.exp.is_index_var())
    {
	int index = C.exp.as_index_var();
	int r = C.lookup_in_env(index);

	// 1a. Evaluate the reg.
	const auto& value = evaluate_reg_to_closure(r);
	// 1b. Put the value on the stack.
	M.closure_stack.push_back(value);
    }
    else if (C.exp.is_atomic())
	// 2. C. is an atomic value, put it on the stack.
	M.closure_stack.push_back(C);
    else
    {
	// 3a. Put the closure on the stack. and 
	M.closure_stack.push_back(C);
	// 3b. Evaluate it to a constant or index_var
	evaluate_stack_to_constant_or_index_var();

	if (M.closure_stack.back().exp.is_index_var())
	{
	    int index = M.closure_stack.back().exp.as_index_var();
	    int r = M.closure_stack.back().lookup_in_env(index);
	    M.closure_stack.pop_back();

	    // 3c. If it evaluates to an index_var, then evaluate the reg.
	    const auto& value = evaluate_reg_to_closure(r);
	    // 3d. Put the value on the stack.
	    M.closure_stack.push_back(value);
	}
    }
}

int OperationArgs::allocate(closure&& C)
{
    int r = allocate_reg();
    M.set_C(r, std::move(C));
    return r;
}

int OperationArgs::allocate_reg()
{
    int r = M.push_temp_head();
    n_allocated++;
    return r;
}

OperationArgs::OperationArgs(reg_heap& m)
    :M(m)
{ }

OperationArgs::~OperationArgs()
{
    for(int i=0;i<n_allocated;i++)
	M.pop_temp_head();
}
