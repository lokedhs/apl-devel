/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

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

#include "APL_types.hh"
#include "CDR_string.hh"
#include "CharCell.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "UCS_string.hh"
#include "Value.hh"
#include "Workspace.hh"

#define stv_def(x) Value Value:: x(LOC, Value::Value_how_ ## x);
#include "StaticValues.def"

//-----------------------------------------------------------------------------
Value::Value(const char * loc)
   : DynamicObject(loc, &all_values),
     flags(VF_NONE)
{
   init_ravel();

   used_memory += sizeof(*this);
}
//-----------------------------------------------------------------------------
Value::Value(const char * loc, Value_how how)
   : DynamicObject(loc),
     flags(VF_forever)
{
   switch(how)
      {
        case Value_how_Zero ... Value_how_Nine:
             init_ravel();
             new (&get_ravel(0)) IntCell(how);
             return;

        case Value_how_Spc:
             init_ravel();
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             return;

        case Value_how_Idx0:
             init_ravel();
             new (&shape) Shape(0);
             new (&get_ravel(0)) IntCell(0);
             return;

        case Value_how_Str0:
             init_ravel();
             new (&shape) Shape(0);
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             return;

        case Value_how_zStr0:
             init_ravel();
             {
               Value * z = new Value(loc, Value_how_Str0);   // ''
               new (&get_ravel(0)) PointerCell(z);           // ⊂''
             }

        case Value_how_Str0_0:
             init_ravel();
             new (&shape) Shape(ShapeItem(0), ShapeItem(0));
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             return;

        case Value_how_Str3_0:
             init_ravel();
             new (&shape) Shape(ShapeItem(3), ShapeItem(0));
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             return;

        case Value_how_Max:
             init_ravel();
             new (&shape) Shape(1);
             new (&get_ravel(0)) FloatCell(72370055773322621e75);
             return;

        case Value_how_Min:
             init_ravel();
             new (&shape) Shape(1);
             new (&get_ravel(0)) FloatCell(-72370055773322621e75);
             return;

        case Value_how_AV:
             new (&shape) Shape(MAX_AV);
             init_ravel();
             loop(cti, MAX_AV)
                 {
                   const int pos = Avec::get_av_pos(CHT_Index(cti));
                   const Unicode uni = Avec::unicode(CHT_Index(cti));

                   new (&get_ravel(pos))  CharCell(uni);
                 }
             return;

        case Value_how_Max_CT:
             init_ravel();
             new (&shape) Shape(1);
             new (&get_ravel(0)) FloatCell(MAX_QUAD_CT);
             return;

        case Value_how_Max_PP:
             init_ravel();
             new (&shape) Shape(1);
             new (&get_ravel(0)) IntCell(MAX_QUAD_PP);
             return;

        case Value_how_Quad_NLT:
             {
                const char * cp = "En_US.nlt";
                const ShapeItem len = strlen(cp);
                new (&shape) Shape(len);
                init_ravel();
                loop(l, len)   new (&get_ravel(l)) CharCell(Unicode(cp[l]));
             }
             return;

        case Value_how_Quad_TC:
             new (&shape) Shape(3);
             init_ravel();
             new (&get_ravel(0)) CharCell(UNI_ASCII_BS);
             new (&get_ravel(1)) CharCell(UNI_ASCII_CR);
             new (&get_ravel(2)) CharCell(UNI_ASCII_LF);
             return;
      }

   // not reached
   FIXME;
}
//-----------------------------------------------------------------------------
Value::Value(ShapeItem sh, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(sh),
     flags(VF_NONE)
{
const ShapeItem length = init_ravel();

   used_memory += sizeof(*this) + length * sizeof(Cell);
}
//-----------------------------------------------------------------------------
Value::Value(const UCS_string & ucs, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(ucs.size()),
     flags(VF_NONE)
{
const ShapeItem length = init_ravel();

   if (length)
      {
        loop(l, length)   new (&get_ravel(l)) CharCell(ucs[l]);
        used_memory += sizeof(*this) + length * sizeof(Cell);
      }
   else   // empty string: set prototype
      {
        new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
        used_memory += sizeof(*this) + sizeof(Cell);
      }

}
//-----------------------------------------------------------------------------
Value::Value(const CDR_string & ui8, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(ui8.size()),
     flags(VF_NONE)
{
const ShapeItem length = init_ravel();

   if (length)
      {
        loop(l, length)   new (&get_ravel(l)) CharCell(Unicode(ui8[l]));
      }
   else   // empty string: set prototype
      {
        new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
      }

   used_memory += sizeof(*this) + sizeof(Cell);
}
//-----------------------------------------------------------------------------
Value::Value(const char * string, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(strlen(string)),
     flags(VF_NONE)
{
const ShapeItem length = init_ravel();

   loop(e, length)  new (&get_ravel(e)) CharCell(Unicode(string[e]));

   used_memory += sizeof(*this) + length * sizeof(Cell);
}
//-----------------------------------------------------------------------------
Value::Value(const Shape & sh, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(sh),
     flags(VF_NONE)
{
const ShapeItem length = init_ravel();

   used_memory += sizeof(*this) + length * sizeof(Cell);
}
//-----------------------------------------------------------------------------
Value::Value(const Cell & cell, const char * loc)
   : DynamicObject(loc, &all_values),
     flags(VF_NONE)
{
   init_ravel();

   get_ravel(0).init(cell);
   used_memory += sizeof(*this) + sizeof(Cell);
}
//-----------------------------------------------------------------------------
Value::~Value()
{
const ShapeItem length = nz_element_count();

Cell * cZ = &get_ravel(0);
   loop(c, length)   cZ++->release(LOC);

   used_memory -= sizeof(*this) + length * sizeof(Cell);
   if (ravel != short_value)  delete ravel;
}
//-----------------------------------------------------------------------------
Value_P
Value::get_cellrefs(const char * loc)
{
Value_P ret = new Value(get_shape(), loc);
   ret->set_left();

const ShapeItem ec = nz_element_count();

   loop(e, ec)
      {
        Cell & cell = get_ravel(e);
        new (&ret->get_ravel(e))   LvalCell(&cell);
      }

   return ret;
}
//-----------------------------------------------------------------------------
void
Value::assign_cellrefs(Value_P new_value)
{
const ShapeItem dest_count = nz_element_count();
const ShapeItem value_count = new_value->nz_element_count();
const Cell * src = &new_value->get_ravel(0);
Cell * C = &get_ravel(0);

const int src_incr = new_value->is_skalar() ? 0 : 1;

   if (is_skalar())
      {
        Assert(C->is_lval_cell());
        Cell * dest = C->get_lval_value();   // can be 0!
        if (dest && dest->is_pointer_cell())
           {
             // we override a pointer cell. erase its value before assigning
             // a new cell
             //
             Value_P old = dest->get_pointer_value();
             old->clear_nested();
             old->erase(LOC);
           }

        if (new_value->is_skalar())
           {
             dest->init(new_value->get_ravel(0));
           }
        else
           {
             new_value->set_shared();
             new (dest)   PointerCell(new_value);
           }
        this->erase(LOC);
        return;
      }


   loop(d, dest_count)
      {
        Assert(C->is_lval_cell());

        Cell * dest = C++->get_lval_value();   // can be 0!
        if (dest && dest->is_pointer_cell())
           {
             // we override a pointer cell. erase its value before assigning
             // a new cell
             //
             Value_P old = dest->get_pointer_value();
             old->clear_nested();
             old->erase(LOC);
           }

        // erase the pointee when overriding a pointer-cell.
        //
        if (dest)   dest->init(*src);
        src += src_incr;
      }

   this->erase(LOC);
}
//-----------------------------------------------------------------------------
bool
Value::is_lval() const
{
const ShapeItem ec = element_count();

   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (cell.is_lval_cell())   return true;
        if (cell.is_pointer_cell() && cell.get_pointer_value()->is_lval())
           return true;

        return false;
      }

   return false;
}
//-----------------------------------------------------------------------------
void
Value::flag_info(const char * loc, ValueFlags flag, const char * flag_name,
                 bool set) const
{
const char * sc = set ? " SET " : " CLEAR ";
const int new_flags = set ? flags | flag : flags & ~flag;
const char * chg = flags == new_flags ? " (no change)" : " (changed)";

   CERR << "Value " << (const void *)this
        << sc << flag_name << " (" << (const void *)flag << ")"
        << " at " << loc << " now = " << (const void *)(intptr_t)new_flags
        << chg << endl;
}
//-----------------------------------------------------------------------------
void
Value::init()
{
   Log(LOG_startup)
      CERR << "Max. Rank            is " << MAX_RANK << endl
           << "sizeof(Value header) is " << sizeof(Value)  << " bytes" << endl
           << "Cell size            is " << sizeof(Cell)   << " bytes" << endl;
};
//-----------------------------------------------------------------------------
void
Value::mark_all_dynamic_values()
{
   for (const DynamicObject * dob = DynamicObject::all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         Value * val = (Value *)dob;
         if (!val->is_forever())   val->set_marked();
       }
}
//-----------------------------------------------------------------------------
void
Value::unmark() const
{
   clear_marked();

const ShapeItem ec = nz_element_count();
const Cell * C = &get_ravel(0);
   loop(e, ec)
      {
        if (C->is_pointer_cell())   C->get_pointer_value()->unmark();
        ++C;
      }
}
//-----------------------------------------------------------------------------
void
Value::set_on_stack()
{
   set_shared();
   unlink();
}
//-----------------------------------------------------------------------------
void
Value::erase(const char * loc) const
{
   if (is_deleted())
      {
        TestFiles::apl_error(LOC);
        CERR                                                           << endl
                                                                       << endl
             << "*** Value " << (const void *)this
             << " deleted twice! ***"                                  << endl
             << "    flags " << get_flags()                            << endl
             << "    This delete at: "  << loc                         << endl
             << "    First delete at: " << where_allocated()           << endl
                                                                       << endl;
        Backtrace::show(__FILE__, __LINE__);
        return;
      }

   if (flags & VF_DONT_DELETE)   return;

   ((Value *)this)->unlink();

   set_deleted();

   Log(LOG_delete)
      {
        const void * addr = this;
        const void * ravel_0 = &get_ravel(0);
        const void * ravel_N = get_ravel_end();
        CERR << "delete " << addr
             << " ravel " << ravel_0 << "-" << ravel_N 
             << " flags " << get_flags()
             << " alloc(" << where_allocated() 
             << ") caller(" << loc << ")" << endl;
      }

   ((Value *)this)->alloc_loc = loc;
   delete this;
}
//-----------------------------------------------------------------------------
void
Value::erase_all(ostream & out)
{
   for (DynamicObject * vb = DynamicObject::all_values.get_next();
        vb != &DynamicObject::all_values; vb = vb->get_next())
       {
         Value * v = (Value *)vb;
         out << "erase_all sees Value:" << endl
             << "  Allocated by " << v->where_allocated() << endl
             << "  ";
         v->list_one(CERR, false);
       }
}
//-----------------------------------------------------------------------------
int
Value::erase_stale(const char * loc)
{
int count = 0;

   Log(LOG_Value__erase_stale)
      CERR << endl << endl << "erase_stale() called from " << loc << endl;

   for (DynamicObject * vb = all_values.get_next();
        vb != &all_values; vb = vb->get_next())
       {
         if (vb == vb->get_next())   // a loop
            {
              CERR << "A loop in DynamicObject::all_values (detected at "
                   << (const void *)vb << "): " << endl;
              all_values.print_chain(CERR);
              CERR << endl;

              CERR << " DynamicObject: " << vb << endl;
              CERR << " Value:         " << (Value *)vb << endl;
              CERR << *(Value *)vb << endl;
            }

         Assert(vb != vb->get_next());
         Value * v = (Value *)vb;
         if (v->flags & VF_DONT_DELETE)   continue;

         Log(LOG_Value__erase_stale)
            {
              CERR << "Erasing stale Value " << (const void *)vb << ":" << endl
                   << "  Allocated by " << v->where_allocated() << endl
                   << "  ";
              v->list_one(CERR, false);
            }

         vb->unlink();
         v->erase(loc);
         ++count;

         // v->erase(loc) could mess up the chain, so we start over
         // rather than continuing
         //
         vb = &all_values;
       }

   return count;
}
//-----------------------------------------------------------------------------
ostream &
Value::list_one(ostream & out, bool show_owners)
{
   if (flags)
      {
        out << "   Flags =";
        char sep = ' ';
        if (is_shared())     { out << sep << "SHARED";     sep = '+'; }
        if (is_assigned())   { out << sep << "ASSIGNED";   sep = '+'; }
        if (is_forever())    { out << sep << "FOREVER";    sep = '+'; }
        if (is_nested())     { out << sep << "NESTED";     sep = '+'; }
        if (is_index())      { out << sep << "INDEX";      sep = '+'; }
        if (is_deleted())    { out << sep << "DELETED";    sep = '+'; }
        if (is_arg())        { out << sep << "ARG";        sep = '+'; }
        if (is_eoc())        { out << sep << "EOC";        sep = '+'; }
        if (is_left())       { out << sep << "LEFT";       sep = '+'; }
        if (is_marked())     { out << sep << "MARKED";     sep = '+'; }
        out << ",";
      }
   out << " Rank = " << get_rank();
   if (get_rank())
      {
        out << ", Shape =";
        loop(r, get_rank())   out << " " << get_shape_item(r);
      }

   out << ":" << endl;
   print(out);
   out << endl;
   if (!show_owners)   return out;

   // print owners...
   //
   out << "Owners of " << (const void *)(intptr_t)this << ":" << endl;

   // static variables
   //
#define stv_def(x) \
   if (this == & x)   out << "(static) Value::" #x << endl;
#include "StaticValues.def"

   Workspace::the_workspace->show_owners(out, this);

   out << "---------------------------" << endl << endl;
   return out;
}
//-----------------------------------------------------------------------------
ostream &
Value::list_all(ostream & out, bool show_owners)
{
int num = 0;
   for (DynamicObject * vb = all_values.get_prev();
        vb != &all_values; vb = vb->get_prev())
       {
         out << "Value #" << num++ << ":";
         ((Value *)vb)->list_one(out, show_owners);
       }

   return out << endl;
}
//-----------------------------------------------------------------------------
ShapeItem
Value::get_enlist_count() const
{
const ShapeItem ec = element_count();
ShapeItem count = ec;

   loop(c, ec)
       {
         const Cell & cell = get_ravel(c);
         if (cell.is_pointer_cell())
            {
               count--;
               count += cell.get_pointer_value()->get_enlist_count();
            }
         else if (cell.is_lval_cell())
            {
              Cell * cp = cell.get_lval_value();
              if (cp && cp->is_pointer_cell())
                 {
                   count--;
                   count += cp->get_pointer_value()->get_enlist_count();
                 }
              else CERR << "non-pointer at " LOC << endl;
            }
       }

   return count;
}
//-----------------------------------------------------------------------------
void Value::enlist(Cell * & dest, bool left) const
{
ShapeItem ec = element_count();

   loop(c, ec)
       {
         const Cell & cell = get_ravel(c);
         if (cell.is_pointer_cell())
            {
              cell.get_pointer_value()->enlist(dest, left);
            }
         else if (left && cell.is_lval_cell())
            {
              const Cell * cp = cell.get_lval_value();
              if (cp && cp->is_pointer_cell())
                 cp->get_pointer_value()->enlist(dest, left);
              else CERR << "non-pointer at " LOC << endl;
            }
         else if (left)
            {
              new (dest++) LvalCell((Cell *)&cell);
            }
         else
            {
              dest++->init(cell);
            }
       }
}
//-----------------------------------------------------------------------------
bool
Value::is_apl_char_vector() const
{
   if (get_rank() != 1)   return false;

   loop(c, get_shape_item(0))
      {
        if (!get_ravel(c).is_character_cell())   return false;

        const Unicode uni = get_ravel(c).get_char_value();
        if (Avec::find_char(uni) == Invalid_CHT)   return false;   // not in ⎕AV
      }
   return true;
}
//-----------------------------------------------------------------------------
bool
Value::is_char_vector() const
{
   if (get_rank() != 1)   return false;

   loop(c, get_shape_item(0))
      if (!get_ravel(c).is_character_cell())   return false;
   return true;
}
//-----------------------------------------------------------------------------
bool
Value::is_char_skalar() const
{
   if (get_rank() != 0)   return false;

   return get_ravel(0).is_character_cell();
}
//-----------------------------------------------------------------------------
bool
Value::NOTCHAR() const
{
   // always test element 0.
   if (!get_ravel(0).is_character_cell())   return true;

const ShapeItem ec = element_count();
   loop(c, ec)   if (!get_ravel(c).is_character_cell())   return true;

   // all items are single chars.
   return false;
}
//-----------------------------------------------------------------------------
int
Value::toggle_UCS()
{
const ShapeItem ec = nz_element_count();
int error_count = 0;

   loop(e, ec)
      {
        Cell & cell = get_ravel(e);
        if (cell.is_character_cell())
           {
             new (&cell)   IntCell(Unicode(cell.get_char_value()));
           }
        else if (cell.is_integer_cell())
           {
             new (&cell)   CharCell(Unicode(cell.get_int_value()));
           }
        else if (cell.is_pointer_cell())
           {
             error_count += cell.get_pointer_value()->toggle_UCS();
           }
        else
           {
             ++error_count;
           }
      }

   return error_count;
}
//-----------------------------------------------------------------------------
bool
Value::is_int_vector(APL_Float qct) const
{
   if (get_rank() != 1)   return false;

   loop(c, get_shape_item(0))
       {
         const Cell & cell = get_ravel(c);
         if (!cell.is_near_int(qct))   return false;
       }

   return true;
}
//-----------------------------------------------------------------------------
bool
Value::is_int_skalar(APL_Float qct) const
{
   if (get_rank() != 0)   return false;

   return get_ravel(0).is_near_int(qct);
}
//-----------------------------------------------------------------------------
bool
Value::is_complex(APL_Float qct) const
{
const ShapeItem ec = nz_element_count();

   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (!cell.is_numeric())        DOMAIN_ERROR;
        if (!cell.is_near_real(qct))   return true;
      }

   return false;   // all cells numeric and not complex
}
//-----------------------------------------------------------------------------
Depth
Value::compute_depth() const
{
   if (is_skalar())
      {
        if (get_ravel(0).is_pointer_cell())
           {
             Depth d = get_ravel(0).get_pointer_value()->compute_depth();
             return 1 + d;
           }

        return 0;
      }

ShapeItem count = nz_element_count();

Depth sub_depth = 0;
   loop(c, count)
       {
         Depth d = 0;
         if (get_ravel(c).is_pointer_cell())
            {
              d = get_ravel(c).get_pointer_value()->compute_depth();
            }
         if (sub_depth < d)   sub_depth = d;
       }

   return sub_depth + 1;
}
//-----------------------------------------------------------------------------
CellType
Value::compute_cell_types() const
{
int32_t ctypes = CT_NONE;

const int64_t count = element_count();
   loop(c, count)
      {
       const Cell & cell = get_ravel(c);
        ctypes |= cell.get_cell_type();
        if (cell.is_pointer_cell())
           ctypes |= cell.get_pointer_value()->compute_cell_types();
      }

   return CellType(ctypes);
}
//-----------------------------------------------------------------------------
Value_P
Value::index(const IndexExpr & IX)
{
   Assert(IX.value_count() != 1);   // should have called index(Value_P X)

   if (get_rank() != IX.value_count())   RANK_ERROR;

   // Notes:
   //
   // 1.  IX is parsed from right to left:    B[I2;I1;I0]  --> I0 I1 I2
   //     the shapes of this and IX are then related as follows:
   //
   //     this     IX
   //     ---------------
   //     0        rank-1   (rank = IX->value_count())
   //     1        rank-2
   //     ...      ...
   //     rank-2   1
   //     rank-1   0
   //     ---------------
   //
   // 2.  shape Z is the concatenation of all shapes in IX
   // 3.  rank Z is the sum of all ranks in IX

   // construct result rank_Z and shape_Z.
   // We go from higher indices of IX to lower indices (Note 1.)
   //
Shape shape_Z;
   loop(this_r, get_rank())
       {
         const ShapeItem idx_r = get_rank() - this_r - 1;

         Value_P I = IX.values[idx_r];
         if (I)
            {
              loop(s, I->get_rank())
                shape_Z.add_shape_item(I->get_shape_item(s));
            }
         else
            {
              shape_Z.add_shape_item(this->get_shape_item(this_r));
            }
       }

MultiIndexIterator mult(get_shape(), IX);   // deletes IDX

   // construct iterators.
   // We go from lower indices to higher indices in IX, which
   // means from higher indices to lower indices in this and Z
   
Value_P Z = new Value(shape_Z, LOC);

const ShapeItem ec_z = Z->element_count();

   loop(z, ec_z)   Z->get_ravel(z).init(get_ravel(mult.next()));

   Assert(mult.done());

   Z->set_default(this);
   CHECK(Z, LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Value::index(Value_P X)
{
   if (get_rank() != 1)   RANK_ERROR;

   if (X == 0)   return clone(LOC);

const Shape shape_Z(X->get_shape());
Value_P Z = new Value(shape_Z, LOC);
const ShapeItem ec = shape_Z.nz_element_count();
const ShapeItem max_idx = element_count();
const APL_Integer qio = Workspace::get_IO();
const APL_Float qct = Workspace::get_CT();

Cell * cZ = &Z->get_ravel(0);
const Cell * cI = &X->get_ravel(0);

   loop(z, ec)
      {
         const ShapeItem idx = cI++->get_near_int(qct) - qio;
         if (idx < 0)          INDEX_ERROR;
         if (idx >= max_idx)   INDEX_ERROR;
         cZ++->init(get_ravel(idx));
      }

   X->erase(LOC);

   CHECK(Z, LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Value::index(Token & IX)
{
   if (IX.get_tag() == TOK_AXES)     return index(IX.get_apl_val());
   if (IX.get_tag() == TOK_INDEX)   return index(IX.get_index_val());

   // not supposed to happen
   //
   Q(IX.get_tag());  FIXME;
}
//-----------------------------------------------------------------------------
Rank
Value::get_single_axis(Rank max_axis) const
{
   if (this == 0)   AXIS_ERROR;

const APL_Float   qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!is_skalar_or_len1_vector())     AXIS_ERROR;

   if (!get_ravel(0).is_near_int(qct))   AXIS_ERROR;

   // if axis becomes (signed) negative then it will be (unsigned) too big.
   // Therefore we need not test for < 0.
   //
const unsigned int axis = get_ravel(0).get_near_int(qct) - qio;
   if (axis >= max_axis)   AXIS_ERROR;

   return axis;
}
//-----------------------------------------------------------------------------
Shape
Value::to_shape() const
{
   if (this == 0)   INDEX_ERROR;   // elided index ?

const ShapeItem xlen = element_count();
const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

Shape shape;
     loop(x, xlen)
        shape.add_shape_item(get_ravel(x).get_near_int(qct) - qio);

   return shape;
}
//-----------------------------------------------------------------------------
/**
   combine two APL values. If a value is closed, then enclose
   it in a (open) skalar.
 **/
Value_P
Value::glue(Token & token, Token & token_A, Token & token_B, const char * loc)
{
Value_P A = token_A.get_apl_val();
Value_P B = token_B.get_apl_val();

const ShapeItem ec_A = A->element_count();
const ShapeItem ec_B = B->element_count();

Value_P Z = 0;

   Log(LOG_glue)
      {
        CERR << "glueing " << A << " to " << B << endl;
        Q(token_A)   Q(token_A.get_tag())
        Q(token_B)   Q(token_B.get_tag())
      }

   if (token_A.get_tag() == TOK_APL_VALUE1)         // A is closed.
      {
         if (token_B.get_tag() == TOK_APL_VALUE1)   // B is closed.
            {
               // A and B are closed
               //
               Z = new Value(2, LOC);
               new (&Z->get_ravel(0))   PointerCell(A);
               new (&Z->get_ravel(1))   PointerCell(B);
            }
         else                                       // B is open
            {
               // A is closed, B is open
               //
               if (B->get_rank() > 1)   RANK_ERROR;
               Z = new Value(1 + ec_B, LOC);
               Cell * cZ = &Z->get_ravel(0);
               const Cell * cB = &B->get_ravel(0);
               new (cZ++)   PointerCell(A);
               loop(e, ec_B)   cZ++->init(*cB++);
            }
      }
   else                                             // A is open
      {
         if (token_B.get_tag() == TOK_APL_VALUE1)   // B is closed.
            {
               // A is open, B is closed
               //
               if (A->get_rank() > 1)   RANK_ERROR;
               Z = new Value(ec_A + 1, LOC);
               Cell * cZ = &Z->get_ravel(0);
               const Cell * cA = &A->get_ravel(0);
               loop(e, ec_A)   cZ++->init(*cA++);
               new (cZ++)   PointerCell(B);
            }
         else                                       // B is open
            {
               // A and B are open
               //
               if (A->get_rank() > 1)   RANK_ERROR;
               if (B->get_rank() > 1)   RANK_ERROR;
               Z = new Value(ec_A + ec_B, LOC);
               Cell * cZ = &Z->get_ravel(0);
               const Cell * cA = &A->get_ravel(0);
               loop(e, ec_A)   cZ++->init(*cA++);
               const Cell * cB = &B->get_ravel(0);
               loop(e, ec_B)   cZ++->init(*cB++);
            }
      }

   Z->set_default(A);

   Log(LOG_glue)
      {
        Q(*A)   Q(A->element_count())
        Q(*B)   Q(B->element_count())
        Q(*Z)   Q(Z->element_count())
        Q("----------------")

        CERR << "erasing A at " << A << " flags " << A->get_flags() << endl
             << "erasing B at " << B << " flags " << B->get_flags() << endl;
      }

   A->erase(LOC);
   B->erase(LOC);

   // invalidate token_A and token_B
   //
   new (&token_A) Token;
   new (&token_B) Token;

   token = Token(TOK_APL_VALUE, Z);
   return Z;
}
//-----------------------------------------------------------------------------
Token
Value::check_value(const char * loc)
{
uint32_t error_count = 0;
const uint64_t count = element_count();

const CellType ctype = get_ravel(0).get_cell_type();
   switch(ctype)
      {
        case CT_CHAR:
        case CT_POINTER:
        case CT_CELLREF:
        case CT_INT:
        case CT_FLOAT:
        case CT_COMPLEX:   break;   // OK

        default:
        CERR << endl
             << "*** check_value(" << loc << ") detects:" << endl
             << "   bad ravel[0] (CellType " << ctype << ")" << endl;
        ++error_count;
      }

   loop(c, count)
       {
         const Cell * cell = &get_ravel(c);
         const CellType ctype = cell->get_cell_type();
          switch(ctype)
             {
               case CT_CHAR:
               case CT_POINTER:
               case CT_CELLREF:
               case CT_INT:
               case CT_FLOAT:
               case CT_COMPLEX:   break;   // OK

               default:
               CERR << endl
                    << "*** check_value(" << loc << ") detects:" << endl
                    << "   bad ravel[" << c << "] (CellType "
                    << ctype << ")" << endl;
               ++error_count;
             }

         if (error_count >= 10)
            {
              CERR << endl << "..." << endl;
              break;
            }
       }

   if (error_count)
      {
        CERR << "Shape: " << get_shape() << endl;
        print(CERR) << endl
             << "************************************************" << endl;
        Assert(0 && "corrupt ravel");
      }

   return Token(TOK_APL_VALUE1, this);
}
//-----------------------------------------------------------------------------
int
Value::total_size_netto(int CDR_type) const
{
   if (CDR_type != 7)   // not nested
      return 16 + 4*get_rank() + data_size(CDR_type);

   // nested: header + offset-array + sub-values.
   //
const ShapeItem ec = nz_element_count();
int size = 16 + 4*get_rank() + 4*ec;   // top_level size
   size = (size + 15) & ~15;           // rounded up to 16 bytes

   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (cell.is_character_cell() || cell.is_numeric())
           {
             // a non-pointer sub value consisting of its own 16 byte header,
             // and 1-16 data bytes, padded up to 16 bytes,
             //
             size += 32;
           }
        else if (cell.is_pointer_cell())
           {
             Value_P sub_val = cell.get_pointer_value();
             const int sub_type = sub_val->get_CDR_type();
             size += sub_val->total_size_brutto(sub_type);
           }
         else
           DOMAIN_ERROR;
      }

   return size;
}
//-----------------------------------------------------------------------------
int
Value::data_size(int CDR_type) const
{
const ShapeItem ec = nz_element_count();
   
   switch (CDR_type) 
      {
        case 0: return (ec + 7) / 8;   // 1/8 byte bit, rounded up
        case 1: return 4*ec;           //   4 byte integer
        case 2: return 8*ec;           //   8 byte float
        case 3: return 16*ec;          // two 8 byte floats
        case 4: return ec;             // 1 byte char
        case 5: return 4*ec;           // 4 byte Unicode char
        case 7: break;                 // nested: continue below.
             
        default: FIXME;
      }      
             
   // compute size of a nested CDR.
   // The top level consists of structural offsets that do not count as data.
   // We therefore simly add up the data sizes for the sub-values.
   //
int size = 0;
   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (cell.is_character_cell() || cell.is_numeric())
           {
             size += cell.CDR_size();
           }
        else if (cell.is_pointer_cell())
           {
             Value_P sub_val = cell.get_pointer_value();
             const int sub_type = sub_val->get_CDR_type();
             size += sub_val->data_size(sub_type);
           }
         else
           DOMAIN_ERROR;
      }

   return size;
}
//-----------------------------------------------------------------------------
int
Value::get_CDR_type() const
{
const ShapeItem ec = nz_element_count();
const Cell & cell_0 = get_ravel(0);

   // if all cells are characters (8 or 32 bit), then return 4 or 5.
   // if all cells are numeric (1, 32, 64, or 128 bit, then return  0 ... 3
   // otherwise return 7 (nested) 

   if (cell_0.is_character_cell())   // 8 or 32 bit characters.
      {
        bool big = false;   // assume 8-bit char
        loop(e, ec)
           {
             const Cell & cell = get_ravel(e);
             if (!cell.is_character_cell())   return 7;
             const Unicode uni = cell.get_char_value();
             if (uni < 0)      big = true;
             if (uni >= 256)   big = true;
           }

        return big ? 5 : 4;   // 8-bit or 32-bit char
      }

   if (cell_0.is_numeric())
      {
        bool is_bool    = false;
        bool is_int     = false;
        bool is_float   = false;
        bool is_complex = false;

        loop(e, ec)
           {
             const Cell & cell = get_ravel(e);
             if (cell.is_integer_cell())
                {
                  const APL_Integer i = cell.get_int_value();
                  const uint32_t high = i >> 32;
                  if (i == 0)                    is_bool  = true;
                  else if (i == 1)               is_bool  = true;
                  else if (high == 0x00000000)   is_int   = true;
                  else if (high == 0xFFFFFFFF)   is_int   = true;
                  else /* too big for int32 */   is_float = true;
                }
             else if (cell.is_float_cell())
                {
                  is_float  = true;
                }
             else if (cell.is_complex_cell())
                {
                  is_complex  = true;
                }
             else return 7;   // mixed: return 7
           }

        if (is_complex)   return 3;
        if (is_float)     return 2;
        if (is_int)       return 1;
        return 0;
      }

   return 7;
}
//-----------------------------------------------------------------------------
ostream &
Value::print(ostream & out) const
{
PrintStyle style;
   if (get_rank() < 2)   // skalar or vector
      {
        style = PR_APL_MIN;
      }
   else                  // matrix or higher
      {
        style = Workspace::the_workspace->get_PS();
        style = PrintStyle(style | PST_NO_FRACT_0);
      }

PrintContext pctx(style, *Workspace::the_workspace);
PrintBuffer pb(*this, pctx);

//   pb.debug(CERR, "Value::print()");

UCS_string ucs(pb, get_rank());

   if (ucs.size() == 0)   return out;

   return out << ucs << endl;
}
//-----------------------------------------------------------------------------
ostream &
Value::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "Addr:    " << (const void *)this << endl
       << ind << "Rank:    " << get_rank()  << endl
       << ind << "Shape:   " << get_shape() << endl
       << ind << "Flags:   " << get_flags();
   if (is_shared())    out << " VF_shared";
   if (is_assigned())  out << " VF_assigned";
   if (is_nested())    out << " VF_nested";
   if (is_index())     out << " VF_index";
   if (is_forever())   out << " VF_forever";
   if (is_arg())       out << " VF_arg";
   if (is_eoc())       out << " VF_eoc";
   if (is_left())      out << " VF_left";
   if (is_marked())    out << " VF_marked";
   if (is_deleted())   out << " VF_deleted";
   out << endl
       << ind << "First:   " << get_ravel(0)  << endl
       << ind << "Dynamic: ";

   DynamicObject::print(out);
   return out;
}
//-----------------------------------------------------------------------------
void
Value::debug(const char * info)
{
const PrintContext pctx(*Workspace::the_workspace);
PrintBuffer pb(*this, pctx);
   pb.debug(CERR, info);
}
//-----------------------------------------------------------------------------
ostream &
Value::print_boxed(ostream & out, const char * info)
{
   if (info)   out << info << endl;

Value_P Z = Quad_CR::fun.do_CR(4, this);
   out << *Z << endl;
   Z->erase(LOC);
   return out;
}
//-----------------------------------------------------------------------------
UCS_string
Value::get_UCS_ravel() const
{
UCS_string ucs;

const ShapeItem ec = element_count();
   loop(e, ec)   ucs += get_ravel(e).get_char_value();

   return ucs;
}
//-----------------------------------------------------------------------------
void
Value::to_varnames(UCS_string * result, bool last) const
{
const ShapeItem var_count = get_rows();
const ShapeItem name_len = get_cols();

   loop(v, var_count)
      {
        ShapeItem nidx = v*name_len;
        UCS_string & name = result[v];
        loop(n, name_len)
           {
             const Unicode uni = get_ravel(nidx++).get_char_value();

             if (n == 0 && uni == UNI_QUAD_QUAD)   // leading ⎕
                {
                  name += uni;
                }
             else if (Avec::is_symbol_char(uni))   // valid symbol char
                {
                  name += uni;
                }
             else if (last)
                {
                  const ShapeItem end = v*name_len;
                  while (nidx < end && get_ravel(nidx++).get_char_value()
                         == UNI_ASCII_SPACE)   ++nidx;
                  if (nidx == end)   break;   // remaining chars are spaces

                  // another name following
                  name.clear();
                  name += get_ravel(nidx++).get_char_value();
                }
             else  
                {
                  break;
                }
           }
      }
}
//-----------------------------------------------------------------------------
void
Value::print_structure(ostream & out, int indent, ShapeItem idx) const
{
   loop(i, indent)   out << "    ";
   if (indent)   out << "[" << idx << "] ";
   out << "addr=" << (const void *)this
       << " ≡" << compute_depth()
       << " ⍴" << get_shape()
       << " flags: " << get_flags() << "   "
       << ((is_marked())   ?  "M" : "-")
       << ((is_left())     ?  "&" : "-")
       << ((is_arg())      ?  "A" : "-")
       << ((is_eoc())      ?  "E" : "-")
       << ((is_deleted())  ?  "∆" : "-")
       << ((is_index())    ?  "⌷" : "-")
       << ((is_nested())   ?  "s" : "-")
       << ((is_forever())  ?  "∞" : "-")
       << ((is_assigned()) ?  "←" : "-")
       << ((is_shared())   ?  "∇" : "-")
       << " " << where_allocated()
       << endl;

const ShapeItem ec = nz_element_count();
const Cell * c = &get_ravel(0);
   loop(e, ec)
      {
        if (c->is_pointer_cell())
           c->get_pointer_value()->print_structure(out, indent + 1, e);
        ++c;
      }
}
//-----------------------------------------------------------------------------
Value_P
Value::prototype(const char * loc) const
{
   // the type of an array is an array with the same structure, but all numbers
   // replaced with 0 and all chars replaced with ' '.
   //
   // the prototype of an array is the type of the first element of the array.

const Cell & first = get_ravel(0);

   if (first.is_pointer_cell())
      {
        Value_P B0 = first.get_pointer_value();
        Value_P Z = new Value(B0->get_shape(), loc);
        const ShapeItem ec_Z =  Z->nz_element_count();

        loop(z, ec_Z)   Z->get_ravel(z).init_type(B0->get_ravel(z));
        return Z;
      }
   else
      {
        Value_P Z = new Value(loc);

        Z->get_ravel(0).init_type(first);
        return Z;
      }
}
//-----------------------------------------------------------------------------
Value_P
Value::clone(const char * loc) const
{
Value_P ret = new Value(get_shape(), loc);

const Cell * src = &get_ravel(0);
Cell * dst = &ret->get_ravel(0);

   Cell::copy(dst, src, nz_element_count());

   return ret;
}
//-----------------------------------------------------------------------------
/// lrp p.138: S←⍴⍴A + NOTCHAR (per column)
int32_t
Value::get_col_spacing(ShapeItem col, bool & not_char) const
{
int32_t max_spacing = 0;
   not_char = false;

const ShapeItem ec = element_count();
const ShapeItem cols = get_last_shape_item();
const ShapeItem rows = ec/cols;
   loop(row, rows)
      {
        int32_t spacing = 1;   // assume simple numeric
        const Cell & cell = get_ravel(col + row*cols);

        if (cell.is_pointer_cell())   // nested 
           {
             Value_P v =  cell.get_pointer_value();
             spacing = v->get_rank();
             if (v->NOTCHAR())
                {
                  not_char = true;
                  ++spacing;
                }
           }
        else if (cell.is_character_cell())   // simple char
           {
             spacing = 0;
           }
        else                                 // simple numeric
           {
             not_char = true;
           }

        if (max_spacing < spacing)   max_spacing = spacing;
      }

   return max_spacing;
}
//-----------------------------------------------------------------------------
int
Value::print_stale(ostream & out)
{
vector<Value_P> stale;

   // first print addresses and remember stale values
   //
   for (const DynamicObject * dob = all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         Value * val = (Value *)dob;
         if (val->flags & VF_DONT_DELETE)   continue;

         out << "stale value at " << (const void *)val << endl;
         stale.push_back(val);
       }

   // then print more info...
   //
   loop(s, stale.size())
      {
        stale[s]->print_stale_info(out, (const DynamicObject *)stale[s]);
       }

int count = stale.size();

   // mark all dynamic values, and then unmark those known in the workspace
   //
   mark_all_dynamic_values();
   Workspace::the_workspace->unmark_all_values();

   // print all values that are still marked
   //
   for (const DynamicObject * dob = all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         Value * val = (Value *)dob;
         if (val->is_forever())   continue;

         // don't print values found in the previous round.
         //
         bool known_stale = false;
         loop(s,  stale.size())
            {
              if (val == stale[s])
                 {
                   known_stale = true;
                   break;
                 }
            }
         if (known_stale)   continue;

         if (val->is_marked())
            {
              val->print_stale_info(out, dob);
              ++count;
              val->unmark();
            }
       }

   return count;
}
//-----------------------------------------------------------------------------
void
Value::print_stale_info(ostream & out, const DynamicObject * dob)
{
   out << "alloc(" << dob->where_allocated()
       << ") flags(" << get_flags() << ")" << endl;

#ifdef REMEMBER_TESTFILE
   if (dob->get_testcase_filename())
      out << "test(" << dob->get_testcase_filename()
          << ":" << dob->get_testcase_lineno() << ")" << endl;
#endif

   try 
      {
        print_structure(out, 0, 0);
        Value_P Z = Quad_CR::fun.do_CR(7, this);
        Z->print(out);
        out << endl;
        Z->erase(LOC);
      }
   catch (...)   { out << " *** corrupt ***"; }

   out << endl;
}
//-----------------------------------------------------------------------------
ostream & operator<<(ostream & out, const Value & v)
{
   v.print(out);
   return out;
}
//-----------------------------------------------------------------------------
