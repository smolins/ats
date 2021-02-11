/*
  The interception fraction evaluator is an algebraic evaluator of a given model.
  
  Generated via evaluator_generator.
*/

#include "interception_fraction_evaluator.hh"
#include "interception_fraction_model.hh"

namespace Amanzi {
namespace SurfaceBalance {
namespace Relations {

// Constructor from ParameterList
InterceptionFractionEvaluator::InterceptionFractionEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariableFieldEvaluator(plist)
{
  Teuchos::ParameterList& sublist = plist_.sublist("interception fraction parameters");
  model_ = Teuchos::rcp(new InterceptionFractionModel(sublist));
  InitializeFromPlist_();
}


// Virtual copy constructor
Teuchos::RCP<FieldEvaluator>
InterceptionFractionEvaluator::Clone() const
{
  return Teuchos::rcp(new InterceptionFractionEvaluator(*this));
}


// Initialize by setting up dependencies
void
InterceptionFractionEvaluator::InitializeFromPlist_()
{
  // Set up my dependencies
  Key domain = Keys::getDomainPrefix(my_key_);

  // my keys
  interception_key_ = Keys::readKey(plist_, domain, "interception", "interception");
  my_keys_.push_back(interception_key_);

  throughfall_rain_key_ = Keys::readKey(plist_, domain, "throughfall and drainage rain", "throughfall_drainage_rain");
  my_keys_.push_back(throughfall_rain_key_);

  throughfall_snow_key_ = Keys::readKey(plist_, domain, "throughfall and drainage snow", "throughfall_drainage_snow");
  my_keys_.push_back(throughfall_snow_key_);


  // - pull Keys from plist
  // dependency: surface-area_index
  ai_key_ = Keys::readKey(plist_, domain, "area index", "area_index");
  dependencies_.insert(ai_key_);
  rain_key_ = Keys::readKey(plist_, domain, "precipitation rain", "precipitation_rain");
  dependencies_.insert(rain_key_);
  snow_key_ = Keys::readKey(plist_, domain, "precipitation snow", "precipitation_snow");
  dependencies_.insert(snow_key_);
  drainage_key_ = Keys::readKey(plist_, domain, "drainage", "drainage");
  dependencies_.insert(drainage_key_);
  air_temp_key_ = Keys::readKey(plist_, domain, "air temperature", "air_temperature");
  dependencies_.insert(air_temp_key_);
}


void
InterceptionFractionEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
          const std::vector<Teuchos::Ptr<CompositeVector> >& results)
{
  Teuchos::RCP<const CompositeVector> ai = S->GetFieldData(ai_key_);
  Teuchos::RCP<const CompositeVector> rain = S->GetFieldData(rain_key_);
  Teuchos::RCP<const CompositeVector> snow = S->GetFieldData(snow_key_);
  Teuchos::RCP<const CompositeVector> drainage = S->GetFieldData(drainage_key_);
  Teuchos::RCP<const CompositeVector> air_temp = S->GetFieldData(air_temp_key_);

  for (CompositeVector::name_iterator comp=results[0]->begin();
       comp!=results[0]->end(); ++comp) {
    const Epetra_MultiVector& ai_v = *ai->ViewComponent(*comp, false);
    const Epetra_MultiVector& rain_v = *rain->ViewComponent(*comp, false);
    const Epetra_MultiVector& snow_v = *snow->ViewComponent(*comp, false);
    const Epetra_MultiVector& drainage_v = *drainage->ViewComponent(*comp, false);
    const Epetra_MultiVector& air_temp_v = *air_temp->ViewComponent(*comp, false);

    Epetra_MultiVector& inter_v = *results[0]->ViewComponent(*comp,false);
    Epetra_MultiVector& tfr_v = *results[1]->ViewComponent(*comp,false);
    Epetra_MultiVector& tfs_v = *results[2]->ViewComponent(*comp,false);

    int ncomp = result->size(*comp, false);
    for (int i=0; i!=ncomp; ++i) {
      double coef = model_->InterceptionFraction(ai_v[0][i]);
      double total_precip = rain_v[0][i] + snow_v[0][i];
      inter_v[0][i] = total_precip * coef;

      double frac_r = total_precip > 0 ? rain_v[0][i] / total_precip :
        (air_temp_v > 273.15 ? 1 : 0);
      tfr_v[0][i] = (1-coef) * rain_v[0][i] + frac_r * drainage[0][i];
      tfs_v[0][i] = (1-coef) * snow_v[0][i] + (1-frac_r) * drainage[0][i];
    }
  }
}


void
InterceptionFractionEvaluator::EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
        Key wrt_key, const Teuchos::Ptr<CompositeVector>& result)
{
  if (wrt_key == drainage_key_) {
    Teuchos::RCP<const CompositeVector> ai = S->GetFieldData(ai_key_);
    Teuchos::RCP<const CompositeVector> rain = S->GetFieldData(rain_key_);
    Teuchos::RCP<const CompositeVector> snow = S->GetFieldData(snow_key_);
    Teuchos::RCP<const CompositeVector> drainage = S->GetFieldData(drainage_key_);
    Teuchos::RCP<const CompositeVector> air_temp = S->GetFieldData(air_temp_key_);

    for (CompositeVector::name_iterator comp=result->begin();
         comp!=result->end(); ++comp) {
      const Epetra_MultiVector& ai_v = *ai->ViewComponent(*comp, false);
      const Epetra_MultiVector& rain_v = *rain->ViewComponent(*comp, false);
      const Epetra_MultiVector& snow_v = *snow->ViewComponent(*comp, false);
      const Epetra_MultiVector& drainage_v = *drainage->ViewComponent(*comp, false);
      const Epetra_MultiVector& air_temp_v = *air_temp->ViewComponent(*comp, false);

      Epetra_MultiVector& inter_v = *results[0]->ViewComponent(*comp,false);
      Epetra_MultiVector& tfr_v = *results[1]->ViewComponent(*comp,false);
      Epetra_MultiVector& tfs_v = *results[2]->ViewComponent(*comp,false);

      int ncomp = result->size(*comp, false);
      for (int i=0; i!=ncomp; ++i) {
        inter_v[0][i] = 0.;

        double frac_r = total_precip > 0 ? rain_v[0][i] / total_precip :
          (air_temp_v > 273.15 ? 1 : 0);
        tfr_v[0][i] = frac_r;
        tfs_v[0][i] = 1-frac_r;
      }
    }
  } else {
    result->PutScalar(0.);
  }
}


} //namespace
} //namespace
} //namespace
