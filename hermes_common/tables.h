// This file is part of Hermes
//
// Hermes is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Hermes is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes; if not, see <http://www.gnu.prg/licenses/>.

#ifndef __HERMES_COMMON_TABLES_H_
#define __HERMES_COMMON_TABLES_H_

#include "common.h"

// Butcher's tables type. The last number in the name always means order,
// the one before last (if provided) is the number of stages.
enum ButcherTableType 
{
   Explicit_RK_1,               // Explicit Runge-Kutta RK-1, or explicit Euler method.
   Implicit_RK_1,               // Implicit Runge-Kutta RK-1, or implicit Euler method.
   Explicit_RK_2,               // Explicit Runge-Kutta RK-2 method.
   Implicit_Crank_Nicolson_2_2, // Implicit Crank_Nicolson method.
   Implicit_SIRK_2_2,           // Implicit SIRK-2-2 method.
   Implicit_ESIRK_2_2,          // Implicit ESIRK-2-2 method.
   Implicit_SDIRK_2_2,          // Implicit SDIRK-2-2 method.
   Implicit_Lobatto_IIIA_2_2,   // Implicit Lobatto IIIA-2 method.
   Implicit_Lobatto_IIIB_2_2,   // Implicit Lobatto IIIB-2 method.
   Implicit_Lobatto_IIIC_2_2,   // Implicit Lobatto IIIB-2 method.
   Explicit_RK_3,               // Explicit Runge-Kutta RK-3 method.
   Explicit_RK_4,               // Explicit Runge-Kutta RK-4 method.
   Implicit_Lobatto_IIIA_3_4,   // Implicit Lobatto IIIA-4 method.
   Implicit_Lobatto_IIIB_3_4,   // Implicit Lobatto IIIB-4 method.
   Implicit_Lobatto_IIIC_3_4,   // Implicit Lobatto IIIB-4 method.
   Implicit_Radau_IIA_3_5,      // Implicit Radau IIA-5 method.
   Implicit_SDIRK_4_5,          // Implicit SDIRK-4-5 method.

   /* EMBEDDED IMPLICIT METHODS */

   Implicit_ESDIRK_TRBDF2_3_23_embedded, // From the paper Analysis and implementation 
                                         // of TR-BDF2 by Hosea and Shampine.
   Implicit_ESDIRK_TRX2_3_23_embedded,   // From the paper Analysis and implementation 
                                         // of TR-BDF2 by Hosea and Shampine.
   Implicit_DIRK_7_45_embedded  // Implicit embedded DIRK method pair of orders four in five (from the paper 
                                // Fudziah Ismail et all: Embedded Pair of Diagonally Implicit Runge-Kutta  
                                // Method for Solving Ordinary Differential Equations). The method has
                                // 7 stages but the first one is explicit. BEWARE - THE B2 ROW IS PROBABLY 
                                // WRONG!
};

// General square table of real numbers.
class HERMES_API Table 
{
public:
  Table();
  Table(unsigned int size);
  virtual void alloc(unsigned int size); 
  unsigned int get_size(); 
  double get_A(unsigned int i, unsigned int j);
  void set_A(unsigned int i, unsigned int j, double val);

protected:
  unsigned int size;
  double** A;
};

/// Butcher's tables for Runge-Kutta methods.
class HERMES_API ButcherTable: public Table
{
public:
  ButcherTable();
  ButcherTable(unsigned int size);
  ButcherTable(ButcherTableType butcher_table);
  virtual void alloc(unsigned int size);
  double get_B(unsigned int i);
  double get_B2(unsigned int i);
  double get_C(unsigned int i);
  void set_B(unsigned int i, double val);
  void set_B2(unsigned int i, double val);
  void set_C(unsigned int i, double val); 
  bool is_explicit();
  bool is_diagonally_implicit();
  bool is_fully_implicit();

protected:
  double* B;
  double* B2;  // This is the second B-row for adaptivity based
               // on embedded R-K methods.
  double* C;
};

#endif
