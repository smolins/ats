/*
  Copyright 2010-202x held jointly by participating institutions.
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

//! Downregulates bare soil evaporation through a dessicated zone.
#include "Teuchos_ParameterList.hpp"
#include "dbc.hh"
#include "evaporation_downregulation_model.hh"
#include "seb_physics_funcs.hh"

namespace Amanzi {
namespace SurfaceBalance {
namespace Relations {

// Constructor from ParameterList
EvaporationDownregulationModel::EvaporationDownregulationModel(Teuchos::ParameterList& plist)
{
  dess_dz_ = plist.get<double>("dessicated zone thickness [m]", 0.1);
  Clapp_Horn_b_ = plist.get<double>("Clapp and Hornberger b of surface soil [-]", 1.0);
  rs_method_ = plist.get<std::string>("Soil resistance method", "sakagucki_zeng");
}

// Constructor from LandCover
EvaporationDownregulationModel::EvaporationDownregulationModel(const LandCover& lc)
{
  dess_dz_ = lc.dessicated_zone_thickness;
  Clapp_Horn_b_ = lc.clapp_horn_b;
  rs_method_ = lc.rs_method;
}

// main method
double
EvaporationDownregulationModel::Evaporation(double sg,
                                            double poro,
                                            double pot_evap,
                                            double sl) const
{
  double rsoil;
  if (rs_method_ == "sellers") {
    rsoil = Relations::EvaporativeResistanceCoefSellers(sl);
  } else {
    rsoil = Relations::EvaporativeResistanceCoefSakaguckiZeng(sg, poro, dess_dz_, Clapp_Horn_b_);
  }
  return pot_evap / (1. + rsoil);
}

double
EvaporationDownregulationModel::DEvaporationDSaturationGas(double sg,
                                                           double poro,
                                                           double pot_evap,
                                                           double sl) const
{
  return 0.;
}

double
EvaporationDownregulationModel::DEvaporationDPorosity(double sg,
                                                      double poro,
                                                      double pot_evap,
                                                      double sl) const
{
  return 0.;
}

double
EvaporationDownregulationModel::DEvaporationDSaturationLiquid(double sg,
                                                              double poro,
                                                              double pot_evap,
                                                              double sl) const
{
  return 0.;
}

double
EvaporationDownregulationModel::DEvaporationDPotentialEvaporation(double sg,
                                                                  double poro,
                                                                  double pot_evap,
                                                                  double sl) const
{
  if (rs_method_ == "sellers") {
    return 1. / (1. + Relations::EvaporativeResistanceCoefSellers(sl));
  } else {
    return 1. / (1. + Relations::EvaporativeResistanceCoefSakaguckiZeng(
                        sg, poro, dess_dz_, Clapp_Horn_b_));
  }
}

} // namespace Relations
} // namespace SurfaceBalance
} // namespace Amanzi
