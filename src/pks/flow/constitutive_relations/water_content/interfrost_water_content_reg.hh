/*
  Copyright 2010-202x held jointly by participating institutions.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors:
*/

#include "interfrost_water_content.hh"

namespace Amanzi {
namespace Flow {
namespace Relations {

Utils::RegisteredFactory<Evaluator, InterfrostWaterContent>
  InterfrostWaterContent::reg_("interfrost water content");

} // namespace Relations
} // namespace Flow
} // namespace Amanzi
