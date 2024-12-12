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

/**********************************************************************************************
 * File         : pref_mshr.cc
 * Author       : Yuxiao Jia, Udit Subramanya
 * Date         : 12/12/2024
 * Description  : MSHR based Prefetcher
 *********************************************************************************************/

#include "global_defs.h"
#include "global_types.h"
#include "debug_macros.h"

#include "utils.h"
#include "assert.h"
#include "uop.h"
#include "core.h"
#include "memory.h"

#include "statistics.h"
#include "pref_mshr.h"
#include "pref_common.h"

#include "all_knobs.h"

///////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_MEM(args...) \
  _DEBUG(*m_simBase->m_knobs->KNOB_DEBUG_MEM_TRACE, ##args)
#define DEBUG(args...) \
  _DEBUG(*m_simBase->m_knobs->KNOB_DEBUG_PREF_MSHR, ##args)

///////////////////////////////////////////////////////////////////////////////////////////////

// Default Constructor
pref_mshr_c::pref_mshr_c(macsim_c *simBase) : pref_base_c(simBase) {
}

// constructor
pref_mshr_c::pref_mshr_c(hwp_common_c *hcc, Unit_Type type,
                             macsim_c *simBase)
  : pref_base_c(simBase) {
  name = "mshr";
  hwp_type = Mem_To_UL1;
  hwp_common = hcc;
  switch (type) {
    case UNIT_SMALL:
      knob_enable = *m_simBase->m_knobs->KNOB_PREF_MSHR_ON;
      break;
    case UNIT_MEDIUM:
      knob_enable = *m_simBase->m_knobs->KNOB_PREF_MSHR_ON_MEDIUM_CORE;
      break;
    case UNIT_LARGE:
      knob_enable = *m_simBase->m_knobs->KNOB_PREF_MSHR_ON_LARGE_CORE;
      break;
  }

}

// destructor
pref_mshr_c::~pref_mshr_c() {
  if (!knob_enable) return;

  delete region_table;
  // delete index_table;
}

// initialization
void pref_mshr_c::init_func(int core_id) {
  if (!knob_enable) return;

  core_id = core_id;

  hwp_info->enabled = true;
  region_table = new mshr_region_table_entry_s[*m_simBase->m_knobs
                                                    ->KNOB_PREF_MSHR_TABLE_N];
                                                  
  std::cout << "mshr init";
}

// L1 hit training function
void pref_mshr_c::l1_hit_func(int tid, Addr lineAddr, Addr loadPC,
                                uop_c *uop, int mshr_matching, int mshr_size) {
  l2_hit_func(tid, lineAddr, loadPC, uop, mshr_matching, mshr_size);
}

// L1 miss training function
void pref_mshr_c::l1_miss_func(int tid, Addr lineAddr, Addr loadPC,
                                 uop_c *uop, int mshr_matching, int mshr_size) {
  l2_miss_func(tid, lineAddr, loadPC, uop, mshr_matching, mshr_size);
}

// L2 hit training function
void pref_mshr_c::l2_hit_func(int tid, Addr lineAddr, Addr loadPC,
                                uop_c *uop, int mshr_matching, int mshr_size) {
  train(tid, lineAddr, loadPC, true, mshr_matching, mshr_size);
}

// L2 miss training function
void pref_mshr_c::l2_miss_func(int tid, Addr lineAddr, Addr loadPC,
                                 uop_c *uop, int mshr_matching, int mshr_size) {
  train(tid, lineAddr, loadPC, false, mshr_matching, mshr_size);
}

// train stride tables
void pref_mshr_c::train(int tid, Addr lineAddr, Addr loadPC, bool l2_hit, int mshr_matching, int mshr_size) {
  // Initialize the region index
  int region_idx = -1;

  // Get the line Index to be prefetched on by shifting the size of log2 Dcache
  Addr lineIndex = lineAddr >> LOG2_DCACHE_LINE_SIZE;

  // get the stride region by shift bit of 8
  Addr index_tag = STRIDE_REGION_MSHR(lineAddr);

  // Define the throttle degree based on the mshrr occupancy
  double throttle_degree = pref_mshr_throttle(mshr_size);
  
  // Loop through the table
  for (int i = 0; i < *m_simBase->m_knobs->KNOB_PREF_MSHR_TABLE_N; ++i) 
  {
      // got a hit in the region table
      if(index_tag == region_table[i].tag && region_table[i].valid) 
      {
        region_idx = i;
        break;
     }
  }
  
  // If it is not in the region table
  if(region_idx < 0){
    
    // Then l2_hit will not mean anything
    if(l2_hit){
      return;
    }

    // If it is not present in the region table, make a new one
    for (int j = 0; j < *m_simBase->m_knobs->KNOB_PREF_MSHR_TABLE_N; ++j) 
    {
      // Check if there are any invalid entry, if found then create new entry
      if (!region_table[j].valid) 
      {
        region_idx = j;
        break;
      }

      // Set the region index to 0 if it is not found at the beginning
      if (region_idx == -1)
      {
        region_idx = 0;
      }

      // Run the LRU in the region table
      if (region_table[region_idx].last_access > region_table[j].last_access)
      {
        region_idx = j;
      }
    }

    // Create a new entry
    create_newentry(region_idx, lineAddr, index_tag);

    // Set the address to be prefetched
    Addr pref_index = lineIndex;

    // Although there is no matching region, we are still prefetching for this address for its high merging rate
    if (mshr_matching > 3) 
      {
        for (int i = 0; i < (int)(2 * throttle_degree); i++)
        { 
            // Sequential addrerss accessing
            pref_index += 1;
            
            if (!hwp_common->pref_addto_l2req_queue(pref_index, hwp_info->id)) 
            {
                break;
            }
        }
      }

    return;
  } 
  // Found in the region table 
  else
  {
    // Update the recent used cycle
    region_table[region_idx].last_access = CYCLE;

    // If found cache line region hit, increase the hit count
    if(l2_hit){

      region_table[region_idx].total_hit += 1;
      return;

    // If cache line not hit
    }else{

      region_table[region_idx].mshr_merging_count += mshr_matching;
      region_table[region_idx].total_accesses += 1;
      Addr pref_index = lineIndex;
      double accuracy = (double)region_table[region_idx].total_hit / (double)region_table[region_idx].total_accesses;

      // The accuracy is too samll with large accesses, so predicts it will no longer need prefetch until its accuracy go up
      // If accuracy is less than 0.2, then not prefetch for this address
      // The degree could be 32, and adjust it if needed with a stride 1
      if(region_table[region_idx].mshr_merging_count > 100 && accuracy < 0.2){
        return;
      }

      // Dynamic prefetching degree based on the mshr merging count
      if (region_table[region_idx].mshr_merging_count > 200) 
      {
        // prefetching for this address with dynamic prefetching degree
        for (int i = 0; i < (int)((region_table[region_idx].mshr_merging_count/25) * throttle_degree) ; i++) 
        {
            // Sequential addrerss accessing
            pref_index += 1; 

            // Do the prefetch
            if (!hwp_common->pref_addto_l2req_queue(pref_index, hwp_info->id)) 
            {
                break; 
            }
        }
      } 

      // If the mshr accesses number is small but with a high mshr merging rate
      else if (mshr_matching > 3) 
      {
        for (int i = 0; i < (int)(4 * throttle_degree); i++) 
        { 
            // Sequential addrerss accessing with stride of 1
            pref_index += 1;

            if (!hwp_common->pref_addto_l2req_queue(pref_index, hwp_info->id)) 
            {
                break;
            }
        }
      }
    }
  }

}

// Create a new region table entry
void pref_mshr_c::create_newentry(int idx, Addr line_addr, Addr region_tag)
{
  region_table[idx].tag = region_tag;
  region_table[idx].valid = true;
  region_table[idx].last_access = CYCLE;
  region_table[idx].mshr_merging_count = 0;
  region_table[idx].total_hit = 0;
  region_table[idx].total_accesses = 0;
}


// Throttle when the mshr's occupancy is high
// Based on differrent mshr occupancy 
double pref_mshr_c::pref_mshr_throttle(int mshr_size)
{
  double mshr_occupancy  = mshr_size/ *m_simBase->m_knobs->KNOB_MEM_MSHR_SIZE;
  double degree = 1;
  if(mshr_occupancy >= 0.7)
  {
    return 0.75;
  }
  else if(mshr_occupancy >= 0.8)
  {
    return 0.5;

  }
  else if(mshr_occupancy >= 0.9)
  {
    return 0.25;

  }
  else if(mshr_occupancy == 1.0)
  {
    return 0;
  }
  else
  {
    return 1;
  }

  return degree;
}