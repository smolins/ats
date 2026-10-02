 /*****************************************************************************
 **
 ** C declarations of the ecosim interface
 **
 ******************************************************************************/

#include "data/BGC_containers.hh"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ecosim_datatest();

void ecosim_setup(
  BGCProperties* properties,
  BGCState* state,
  BGCInternalState* internal_state,
  BGCSizes* sizes,
  int num_iterations,
  int num_columns,
  int ncells_per_col_
);

void ecosim_shutdown();

void ecosim_advance(
  double delta_t,
  BGCProperties* properties,
  BGCState* state,
  BGCInternalState* internal_state,
  BGCSizes* sizes,
  int num_iterations,
  int num_columns
);

/* layout of the EcoSIM internal state; static, callable before ecosim_setup */
int ecosim_internal_state_layout_version();

int ecosim_internal_state_num_entries(const BGCSizes* sizes);

/* i is 0-based; buffers must hold kBGCStateNameLength, kBGCStateNameLength,
   kBGCStateUnitsLength and kBGCStateDescriptionLength characters */
void ecosim_internal_state_entry(int i,
                                 const BGCSizes* sizes,
                                 char* ats_name,
                                 char* ecosim_name,
                                 char* units,
                                 char* description,
                                 int* num_components,
                                 int* role);

#ifdef __cplusplus
}
#endif /* __cplusplus */
