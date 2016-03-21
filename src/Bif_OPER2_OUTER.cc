/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Bif_OPER2_OUTER.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "Workspace.hh"

Bif_JOT          Bif_JOT        ::_fun;
Bif_OPER2_OUTER  Bif_OPER2_OUTER::_fun;

Bif_JOT         * Bif_JOT        ::fun = &Bif_JOT        ::_fun;
Bif_OPER2_OUTER * Bif_OPER2_OUTER::fun = &Bif_OPER2_OUTER::_fun;

Bif_OPER2_OUTER::PJob_product Bif_OPER2_OUTER::job;

//-----------------------------------------------------------------------------
Token
Bif_OPER2_OUTER::eval_ALRB(Value_P A, Token & LO, Token & _RO, Value_P B)
{
   if (!_RO.is_function())    SYNTAX_ERROR;

Function * RO = _RO.get_function();

   if (!RO->has_result())   DOMAIN_ERROR;

   return outer_product(A, _RO, B);
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_OUTER::scalar_outer_product() const
{
#ifdef PERFORMANCE_COUNTERS_WANTED
#ifdef HAVE_RDTSC
const uint64_t start_1 = cycle_counter();
#endif
#endif

  // the empty cases have been ruled out already in outer_product()

const ShapeItem Z_len = job.ZAh * job.ZBl;
   job.ec = E_NO_ERROR;

#if PARALLEL_ENABLED
   if (  Parallel::run_parallel
      && Thread_context::get_active_core_count() > 1
      && Z_len > get_dyadic_threshold())
      {
        job.cores = Thread_context::get_active_core_count();
        Thread_context::do_work = PF_scalar_outer_product;
        Thread_context::M_fork("scalar_outer_product");   // start pool
        PF_scalar_outer_product(Thread_context::get_master());
        Thread_context::M_join();
      }
   else
#endif // PARALLEL_ENABLED
      {
        job.cores = CCNT_1;
        PF_scalar_outer_product(Thread_context::get_master());
      }

#ifdef PERFORMANCE_COUNTERS_WANTED
#ifdef HAVE_RDTSC
const uint64_t end_1 = cycle_counter();
   Performance::fs_OPER2_OUTER_AB.add_sample(end_1 - start_1, Z_len);
#endif
#endif
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_OUTER::PF_scalar_outer_product(Thread_context & tctx)
{
const ShapeItem Z_len = job.ZAh * job.ZBl;

const ShapeItem slice_len = (Z_len + job.cores - 1)/job.cores;
ShapeItem z = tctx.get_N() * slice_len;
ShapeItem end_z = z + slice_len;
   if (end_z > Z_len)   end_z = Z_len;

   for (; z < end_z; ++z)
       {
        const ShapeItem zah = z/job.ZBl;
        const ShapeItem zbl = z - zah*job.ZBl;
        job.ec = ((job.cB + zbl)->*job.RO)(job.cZ + z, job.cA + zah);
        if (job.ec != E_NO_ERROR)   return;
       }
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_OUTER::outer_product(Value_P A, Token & _RO, Value_P B)
{
Function * RO = _RO.get_function();
   Assert(RO);

Value_P Z(A->get_shape() + B->get_shape(), LOC);

   // an important (and the most likely) special case is RO being a scalar
   // function. This case can be implemented in a far simpler fashion than
   // the general case.
   //
   if (RO->get_scalar_f2() && A->is_simple() && B->is_simple())
      {
        job.cZ     = &Z->get_ravel(0);
        job.cA     = &A->get_ravel(0);
        job.ZAh    = A->element_count();
        job.RO     = RO->get_scalar_f2();
        job.cB     = &B->get_ravel(0);
        job.ZBl    = B->element_count();
        job.ec     = E_NO_ERROR;

        scalar_outer_product();
        if (job.ec != E_NO_ERROR)   throw_apl_error(job.ec, LOC);

        Z->set_default(*B.get());
 
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (Z->is_empty())
      {
        Value_P Fill_A = Bif_F12_TAKE::first(A);
        Value_P Fill_B = Bif_F12_TAKE::first(B);

        Value_P Z1 = RO->eval_fill_AB(Fill_A, Fill_B).get_apl_val();
        Z->get_ravel(0).init(Z1->get_ravel(0), Z.getref(), LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

EOC_arg arg(Z, A, 0, RO, B);
OUTER_PROD & _arg = arg.u.u_OUTER_PROD;

   _arg.len_B = B->element_count();
   _arg.len_Z = A->element_count() * _arg.len_B;

   return finish_outer_product(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_OUTER::finish_outer_product(EOC_arg & arg)
{
OUTER_PROD & _arg = arg.u.u_OUTER_PROD;

Value_P RO_A;
Value_P RO_B;

   while (++arg.z < _arg.len_Z)
      {
        const Cell * cA = &arg.A->get_ravel(arg.z / _arg.len_B);
        const Cell * cB = &arg.B->get_ravel(arg.z % _arg.len_B);

        if (cA->is_pointer_cell())
           {
             RO_A = cA->get_pointer_value();
           }
        else
           {
             RO_A = Value_P(LOC);
             RO_A->get_ravel(0).init(*cA, RO_A.getref(), LOC);
           }

        if (cB->is_pointer_cell())
           {
             RO_B = cB->get_pointer_value();
           }
        else
           {
             RO_B = Value_P(LOC);
             RO_B->get_ravel(0).init(*cB, RO_B.getref(), LOC);
           }

        Token result = arg.RO->eval_AB(RO_A, RO_B);

      // if RO was a primitive function, then result may be a value.
      // if RO was a user defined function then result may be
      // TOK_SI_PUSHED. In both cases result could be TOK_ERROR.
      //
      if (result.get_Class() == TC_VALUE)
         {
           Value_P ZZ = result.get_apl_val();
           arg.Z->next_ravel()->init_from_value(ZZ, arg.Z.getref(), LOC);
           continue;
         }

      if (result.get_tag() == TOK_ERROR)   return result;

      if (result.get_tag() == TOK_SI_PUSHED)
         {
           // RO was a user defined function
           //
           Workspace::SI_top()->add_eoc_handler(eoc_OUTER, arg, LOC);
           if (arg.z)   delete &arg;   // subsequent call

           return result;   // continue in user defined function...
         }

        Q1(result);   FIXME;
      }

   arg.Z->set_default(*arg.B.get());

   arg.Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, arg.Z);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_OUTER::eoc_OUTER(Token & token)
{
   if (token.get_Class() != TC_VALUE)  return false;   // stop it

EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
OUTER_PROD & _arg = arg->u.u_OUTER_PROD;

   // the user defined function has returned a value. Store it.
   //
   {
     Value_P ZZ = token.get_apl_val();
     arg->Z->next_ravel()->init_from_value(ZZ, arg->Z.getref(), LOC);
   }

   // pop the SI unless this is the last ravel element of Z to be computed
   //
   if (arg->z < (_arg.len_Z - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_outer_product(*arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   delete arg;
   Workspace::SI_top()->set_eoc_handlers(next);
   if (next)   return next->handler(token);

   return false;   // stop it
}
//-----------------------------------------------------------------------------
