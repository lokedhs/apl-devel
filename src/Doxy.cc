/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2017  Dr. Jürgen Sauermann

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
#include <sys/stat.h>

#include "Doxy.hh"
#include "Heapsort.hh"
#include "PrintOperator.hh"
#include "UserFunction.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

using namespace std;

#define BRLF "<BR>\r\n"
#define CRLF "\r\n"

//-----------------------------------------------------------------------------
/// a helper for sorting Symbol * by symbol name
static bool
symcomp(const Symbol * const & s1, const Symbol * const & s2, const void *)
{
   return s2->get_name().compare(s1->get_name()) == COMP_LT;
}
//-----------------------------------------------------------------------------
Doxy::Doxy(ostream & cout, const UCS_string & dest_dir)
   : out(cout),
     root_dir(dest_dir)
{
   ws_name = Workspace::get_WS_name();
   if (ws_name.compare(UCS_string("CLEAR WS")) == 0)
      ws_name = UCS_string("CLEAR-WS");

   root_dir.append_str("/");
   root_dir.append(UTF8_string(ws_name));

   out << "Creating output directory " << root_dir << endl;
   mkdir(root_dir.c_str(), 0777);

   write_css();
}
//-----------------------------------------------------------------------------
void
Doxy::write_css()
{
UTF8_string css_filename(root_dir);
   css_filename.append_str("/apl_doxy.css");
   out << "Writing style sheet file " << css_filename << endl;

ofstream css(css_filename.c_str());
   css <<
"/* GNU APL Doxy css file */"                                              CRLF
"BODY  { background-color: #B0FFB0 }"                                      CRLF
"TABLE { background-color: #FFFF80 }"                                      CRLF
"H1    { text-align: center }"                                             CRLF
"H2    { text-align: center }"                                             CRLF
".code   { font-family: fixed }"                                           CRLF
"table, th, td { border: 1px solid black }"                                CRLF
".funtab,"                                                                 CRLF
".vartab  { margin-left:auto; margin-right:auto;"                          CRLF
"           border-collapse: collapse; }"                                  CRLF;

   css.close();
}
//-----------------------------------------------------------------------------
void
Doxy::gen()
{
const SymbolTable & symtab = Workspace::get_symbol_table();

Simple_string<const Symbol *, false> all_symbols = symtab.get_all_symbols();
Simple_string<const Symbol *, false> functions;
Simple_string<const Symbol *, false> variables;

   loop(a, all_symbols.size())
      {
        const Symbol * sym = all_symbols[a];
        if (sym->is_erased())                continue;
        if (sym->get_name()[0] == UNI_MUE)   continue;   // macro

        bool is_function = false;
        bool is_variable = false;
        loop(vs, sym->value_stack_size())
            {
              switch((*sym)[vs].name_class)
                 {
                   case NC_VARIABLE: is_variable = true;   break;
                   case NC_FUNCTION: is_function = true;   break;
                   case NC_OPERATOR: is_function = true;   break;
                   default: ;
                 }
            }
        if (is_function)   functions.append(sym);
        if (is_variable)   variables.append(sym);
      }

   Heapsort<const Symbol *>::sort(&functions[0], functions.size(), 0, symcomp);
   Heapsort<const Symbol *>::sort(&variables[0], variables.size(), 0, symcomp);

UTF8_string index_filename(root_dir);
   index_filename.append_str("/index.html");
   out << "Writing top-level HTML file " << index_filename << endl;
ofstream index(index_filename.c_str());
   index <<
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""                      CRLF
"                      \"http://www.w3.org/TR/html4/strict.dtd\">"         CRLF
"<html>"                                                                   CRLF
"  <head>"                                                                 CRLF
"    <title>Documentation of " << ws_name << "</title> "                   CRLF
"    <meta http-equiv=\"content-type\" "                                   CRLF
"          content=\"text/html; charset=UTF-8\">"                          CRLF
"   <link rel='stylesheet' type='text/css' href='apl_doxy.css'>"           CRLF
" </head>"                                                                 CRLF
" <body>"                                                                  CRLF
"  <H1>Workspace " << ws_name << "</H1>"                                   CRLF
"   <H2>Defined Functions</H2>"                                            CRLF
"    <TABLE class=funtab>"                                                 CRLF
"     <TR>"                                                                CRLF
"      <TH>Function"                                                       CRLF
"      <TH>Header"                                                         CRLF;
   loop(f, functions.size())
      {
        const Symbol & fun_sym = *functions[f];
        loop(vs, fun_sym.value_stack_size())
            {
              if (fun_sym[vs].name_class == NC_FUNCTION ||
                  fun_sym[vs].name_class == NC_OPERATOR)
                 {
                   const Function * fp = fun_sym[vs].sym_val.function;
                   const UserFunction * ufun = fp->get_ufun1();
                   Assert(fp);
                   const char * native = "";
                   if (fp->is_native())   native = " (native)";
                   index << "  <tr>"                                       CRLF
                            "   <td class=code>"
                         << fun_sym.get_name() << native <<                CRLF
                            "   <td class=code>";
                   if (ufun && !fp->is_native())   bold_name(index, ufun);
                   else                            index << "-";
                   index <<                                                CRLF;
                 }
            }
      }
   index <<
"    </TABLE>"                                                             CRLF

"   <H2>Variables</H2>"                                                    CRLF
"    <TABLE class=vartab>"                                                 CRLF
"     <TR>"                                                                CRLF
"      <TH>Variable"                                                       CRLF
"      <TH>⍴⍴"                                                             CRLF
"      <TH>⍴"                                                              CRLF
"      <TH>≡"                                                              CRLF;
   loop(v, variables.size())
      {
        const Symbol & var_sym = *variables[v];
        loop(vs, var_sym.value_stack_size())
            {
              if (var_sym[vs].name_class == NC_VARIABLE)
                 {
                   Assert(!!var_sym[vs].apl_val);
                   const Value * value = var_sym[vs].apl_val.get();
                   index << "  <tr>"                                       CRLF
                            "   <td class=code>" << var_sym.get_name() <<  CRLF
                            "   <td class=code>" << value->get_rank() <<   CRLF
                            "   <td class=code>";
                   loop(r, value->get_rank())
                       index << " " << value->get_shape_item(r);
                   index << CRLF << "   <td class=code>"
                         << value->compute_depth()                      << CRLF;
                 }
            }
      }
   index <<
"    </TABLE>"                                                             CRLF
" </body>"                                                                 CRLF;

   index.close();
}
//-----------------------------------------------------------------------------
void
Doxy::bold_name(ostream & of, const UserFunction * ufun)
{
const UserFunction_header & header = ufun->get_header();
const char * bold = "<span style='font-weight: bold'>";

   if (header.Z())   of << header.Z()->get_name() << "←";
   if (header.A())   of << header.A()->get_name() << " ";
   if (header.LO())   // operator
      {
        of << "(" << header.LO()->get_name() << " ";
        of << bold << header.get_name() << "</span>";
        if (header.RO())   of << " " << header.RO()->get_name();
      }
   else               // function
      {
        of << bold << header.get_name() << "</span>";
      }

   if (header.B())   of << " " << header.B()->get_name();
}
//-----------------------------------------------------------------------------
