/*
  Copyright 2010-202x held jointly by participating institutions.
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors:

*/

/*
  WRM which calls another WRM for saturation but sets 0 rel perm.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "wrm_zero_relperm.hh"

namespace Amanzi {
namespace Flow {

Utils::RegisteredFactory<WRM, WRMZeroRelPerm> WRMZeroRelPerm::factory_("zero rel perm");

} // namespace Flow
} // namespace Amanzi
