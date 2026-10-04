/*
  Basic architecture based on the Alquima interfece adapted for
  use in the ATSEcoSIM PK

  Copyright 2010-202x held jointly by LANS/LANL, LBNL, and PNNL.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Jeffrey Johnson
           Sergi Molins <smolins@lbl.gov>

  This implements the Alquimia chemistry engine.
*/

#include <iostream>
#include <cstring>
#include <cstdio>
#include <assert.h>
#include "EcoEngine.hh"
#include "ecosim_interface.h"
#include "errors.hh"
#include "exceptions.hh"

// Support for manipulating floating point exception handling.
#ifdef _GNU_SOURCE
#define AMANZI_USE_FENV
#include <fenv.h>
#endif

namespace Amanzi {
namespace EcoSIM {

BGCEngine::BGCEngine(const std::string& engineName,
                                 const std::string& inputFile) :
  bgc_engine_name_(engineName),
  bgc_engine_inputfile_(inputFile)
{
  Errors::Message msg;

  CreateBGCInterface(bgc_engine_name_.c_str(),
                    &bgc_);

}

BGCEngine::~BGCEngine()
{
  bgc_.Shutdown();

  //Did I forget to implement this?
  //FreeBGCProperties(&props);
  //FreeBGCState(&state);
  //FreeBGCAuxiliaryData(&aux_data);
  //FreeAlquimiaEngineStatus(&chem_status_);
}

const BGCSizes&
BGCEngine::Sizes() const
{
  return sizes_;
}

void BGCEngine::InitState(BGCProperties& properties,
                                BGCState& state,
                                BGCAuxiliaryData& aux_data,
                                int ncells_per_col_,
                                int num_components,
                                int num_columns,
                                int num_pfts)
{
  AllocateBGCProperties(&sizes_, &properties, ncells_per_col_, num_columns,num_pfts);
  AllocateBGCState(&sizes_, &state, ncells_per_col_, num_components, num_columns, num_pfts);
}

// The internal state layout is defined by EcoSIM, so it is queried directly
// rather than through the BGCInterface function table.
int BGCEngine::InternalStateLayoutVersion() const
{
  return ecosim_internal_state_layout_version();
}

std::vector<BGCInternalStateEntry>
BGCEngine::InternalStateLayout(const BGCSizes& sizes) const
{
  std::vector<BGCInternalStateEntry> layout;
  int num_entries = ecosim_internal_state_num_entries(&sizes);
  for (int i = 0; i != num_entries; ++i) {
    char ats_name[kBGCStateNameLength], ecosim_name[kBGCStateNameLength];
    char units[kBGCStateUnitsLength], description[kBGCStateDescriptionLength];
    BGCInternalStateEntry entry;
    ecosim_internal_state_entry(
      i, &sizes, ats_name, ecosim_name, units, description, &entry.num_components, &entry.role);
    entry.ats_name = ats_name;
    entry.ecosim_name = ecosim_name;
    entry.units = units;
    entry.description = description;
    layout.push_back(entry);
  }
  return layout;
}

void BGCEngine::InitInternalState(BGCInternalState& internal_state,
                                  const std::vector<BGCInternalStateEntry>& layout,
                                  int num_columns)
{
  int values_per_column = 0;
  for (const auto& entry : layout) values_per_column += entry.num_components;
  AllocateBGCInternalState(&internal_state, InternalStateLayoutVersion(), layout.size(),
                           num_columns, values_per_column);
}

void BGCEngine::FreeInternalState(BGCInternalState& internal_state)
{
  FreeBGCInternalState(&internal_state);
}

void BGCEngine::FreeState(BGCProperties& properties,
                                BGCState& state,
                                BGCAuxiliaryData& aux_data)
{
  FreeBGCProperties(&properties);
  FreeBGCState(&state);
}

void BGCEngine::DataTest() {

  bgc_.DataTest();
}

bool BGCEngine::Setup(BGCProperties& properties,
                              BGCState& state,
                              BGCInternalState& internal_state,
                              BGCSizes& sizes_,
                              int num_iterations,
                              int num_columns,
                              int ncells_per_col_)
{
  bgc_.Setup(&properties,
                &state,
                &internal_state,
                &sizes_,
                num_iterations,
                num_columns,
                ncells_per_col_);
  
  return true;

}

bool BGCEngine::Advance(const double delta_time,
                              BGCProperties& properties,
                              BGCState& state,
                              BGCInternalState& internal_state,
                              BGCSizes& sizes_,
                              int num_iterations,
                              int num_columns)
{
  bgc_.Advance(delta_time,
                &properties,
                &state,
                &internal_state,
                &sizes_,
                num_iterations,
                num_columns);
  
  return true;

}

} // namespace
} // namespace
