/*
Copyright (c) <2012>, <Georgia Institute of Technology> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials provided
with the distribution.

Neither the name of the <Georgia Institue of Technology> nor the names of its contributors
may be used to endorse or promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/***************************************************************************************
 * File         : pref_mshr.h
 * Author       : Yuxiao Jia, Udit Subramanya
 * Date         : 12/12/2024
 * Description  : MSHR based Prefetcher
 *********************************************************************************************/

#ifndef __PREF_MSHR_H__
#define __PREF_MSHR_H__

#include "pref_common.h"
#include "pref.h"

#define STRIDE_REGION_MSHR(x) \
  (x >> (*m_simBase->m_knobs->KNOB_PREF_MSHR_REGION_BITS))

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief stride region information table entry
///////////////////////////////////////////////////////////////////////////////////////////////
typedef struct mshr_region_table_entry_struct {
  Addr tag;
  bool valid; 
  uns last_access;
  int mshr_merging_count;
  double total_hit;
  int total_accesses;

  /**
   * Constructor
   */
  mshr_region_table_entry_struct() {
    valid = false;
  }
} mshr_region_table_entry_s;


///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief MSHR based prefetcher
///
/// MSHR based prefetcher. For more detailed information, refer to pref_base_c class
/// @see pref_base_c
///////////////////////////////////////////////////////////////////////////////////////////////
class pref_mshr_c : public pref_base_c
{
  friend class pref_common_c;

public:
  /**
   * Default constructor
   */
  pref_mshr_c(macsim_c *simBase);

  /**
   * Constructor
   */
  pref_mshr_c(hwp_common_c *, Unit_Type, macsim_c *simBase);

  /**
   * Destructor
   */
  ~pref_mshr_c();

  /**
   * Init function
   */
  void init_func(int);

  /**
   * Done function
   */
  void done_func() {
  }

  /**
   * L1 miss function
   */
  void l1_miss_func(int, Addr, Addr, uop_c *, int mshr_matching, int mshr_size);

  /**
   * L1 hit function
   */
  void l1_hit_func(int, Addr, Addr, uop_c *, int mshr_matching, int mshr_size);

  /**
   * L1 prefetch hit function
   */
  void l1_pref_hit_func(int, Addr, Addr, uop_c *) {
  }

  /**
   * L2 miss function
   */
  void l2_miss_func(int, Addr, Addr, uop_c *, int mshr_matching, int mshr_size);

  /**
   * L2 hit function
   */
  void l2_hit_func(int, Addr, Addr, uop_c *, int mshr_matching, int mshr_size);

  /**
   * L2 prefetch hit function
   */
  void l2_pref_hit_func(int, Addr, Addr, uop_c *) {
  }

  /**
   * Train stride table
   */
  void train(int, Addr, Addr, bool, int mshr_matching, int mshr_size);

  /**
   * Create a new stride
   */
  void create_newentry(int idx, Addr line_addr, Addr region_tag);

  double pref_mshr_throttle(int mshr_size);

private:
  mshr_region_table_entry_s *region_table; /**< address region info */
};

#endif
