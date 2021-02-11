/*
  Drainage rate.

  License: BSD
  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "drainage_evaluator.hh"

namespace Amanzi {
namespace SurfaceBalance {
namespace Relations {

DrainageEvaluator::DrainageEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariablesFieldEvaluator(plist)
{
  std::string domain = Keys::getDomainName(my_key_);

  drainage_key_ = Keys::getKey(plist_, domain, "drainage", "drainage");
  my_keys_.push_back(drainage_key_);

  fracwet_key_ = Keys::getKey(plist_, domain, "fraction wet", "fracwet");
  my_keys_.push_back(fracwet_key_);

  // Set up my dependencies.
  // -- the extent of material, LAI for
  ai_key_ = Keys::getKey(plist_, domain, "area index", "area_index");
  dependencies_.insert(ai_key_);

  // -- water content of the layer drained
  wc_key_ = Keys::getKey(plist_, domain, "water equivalent", "water_equivalent");
  dependencies_.insert(wc_key_);

  // parameters for the drainage model
  tau_ = plist_.get<double>("drainage timescale [s]", 864);

  // default from Dickinson et al 93, CLM 4.5 Tech note eqn 7.8
  wc_sat_ = plist_.get<double>("saturated specific water content [m^3 H2O / m^2 leaf area]", 1.e-4);
};


Teuchos::RCP<FieldEvaluator> DrainageEvaluator::Clone() const
{
  return Teuchos::rcp(new DrainageEvaluator(*this));
}


void DrainageEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
          const std::vector<Teuchos::Ptr<CompositeVector> >& results)
{
  // Pull dependencies out of state.
  const Epetra_MultiVector& wc =
      *S->GetFieldData(wc_key_)->ViewComponent("cell",false);
  const Epetra_MultiVector& ai =
      *S->GetFieldData(ai_key_)->ViewComponent("cell",false);

  Epetra_MultiVector& res_drainage_c = *results[0]->ViewComponent("cell",false);
  Epetra_MultiVector& res_fracwet_c = *results[1]->ViewComponent("cell",false);

  // evaluate the model
  for (int c=0; c!=res_c.MyLength(); ++c) {
    double wc_sat = ai[0][c] * wc_sat_;
    res_fracwet_c[0][c] = wc_sat > 0. ? wc[0][c] / wc_sat : 0.;
    if (wc[0][c] > wc_sat) {
      //  is oversaturated and draining
      res_drainage_c[0][c] = (wc[0][c] - wc_sat) / tau_;
    } else {
      res_drainage_c[0][c] = 0.;
    }
  }
}


void
DrainageEvaluator::EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
        Key wrt_key, const std::vector<Teuchos::Ptr<CompositeVector> > & results)
{
  // Pull dependencies out of state.
  const Epetra_MultiVector& wc =
      *S->GetFieldData(wc_key_)->ViewComponent("cell",false);
  const Epetra_MultiVector& ai =
      *S->GetFieldData(ai_key_)->ViewComponent("cell",false);
  const Epetra_MultiVector& cv =
      *S->GetFieldData("surface-cell_volume")->ViewComponent("cell",false);

  Epetra_MultiVector& res_drainage_c = *results[0]->ViewComponent("cell",false);
  Epetra_MultiVector& res_fracwet_c = *results[1]->ViewComponent("cell",false);

  if (wrt_key == wc_key_) {
    for (int c=0; c!=res_c.MyLength(); ++c) {
      double wc_sat = ai[0][c] * wc_sat_;
      res_fracwet_c[0][c] = wc_sat > 0. ? 1/wc_sat : 0;
      if (wc[0][c] > wc_sat) {
        //  is oversaturated and draining
        res_c[0][c] = 1.0 / tau_;
      } else {
        res_c[0][c] = 0.;
      }
    }
  } else {
    AMANZI_ASSERT(0);
  }

}

} // namespace
} // namespace
} // namespace
