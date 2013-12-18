#include "computation/computation.H"
#include "bounds.H"

extern "C" closure builtin_function_get_bounds(OperationArgs& Args)
{
  auto L = Args.evaluate(0);
  auto U = Args.evaluate(1);

  auto has_lower = boost::dynamic_pointer_cast<const Double>(L);
  auto has_upper = boost::dynamic_pointer_cast<const Double>(U);

  double lower = 0;
  double upper = 0;

  if (has_lower)
    lower = *has_lower;
  if (has_upper)
    upper = *has_upper;
  
  return Bounds<double>((bool)has_lower, lower, (bool)has_upper, upper);
}

extern "C" closure builtin_function_get_integer_bounds(OperationArgs& Args)
{
  auto L = Args.evaluate(0);
  auto U = Args.evaluate(1);

  auto has_lower = boost::dynamic_pointer_cast<const Int>(L);
  auto has_upper = boost::dynamic_pointer_cast<const Int>(U);

  int lower = 0;
  int upper = 0;

  if (has_lower)
    lower = *has_lower;
  if (has_upper)
    upper = *has_upper;
  
  return Bounds<int>((bool)has_lower, lower, (bool)has_upper, upper);
}