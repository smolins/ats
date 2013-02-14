/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
  Evaluator for determining height( rho, head )

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef AMANZI_FLOW_RELATIONS_HEIGHT_EVALUATOR_
#define AMANZI_FLOW_RELATIONS_HEIGHT_EVALUATOR_

#include "secondary_variable_field_evaluator.hh"
#include "factory.hh"

namespace Amanzi {
namespace Flow {
namespace FlowRelations {

class HeightEvaluator : public SecondaryVariableFieldEvaluator {

 public:
  // constructor format for all derived classes
  explicit
  HeightEvaluator(Teuchos::ParameterList& plist);
  HeightEvaluator(const HeightEvaluator& other);

  virtual Teuchos::RCP<FieldEvaluator> Clone() const;

  // Needs a special ensure and derivative to get around trying to find face
  // values and derivatives of face values.
  virtual void EnsureCompatibility(const Teuchos::Ptr<State>& S);
  virtual void UpdateFieldDerivative_(const Teuchos::Ptr<State>& S, Key wrt_key);
  
 protected:

  // Required methods from SecondaryVariableFieldEvaluator
  virtual void EvaluateField_(const Teuchos::Ptr<State>& S,
          const Teuchos::Ptr<CompositeVector>& result);
  virtual void EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
          Key wrt_key, const Teuchos::Ptr<CompositeVector>& result);

 protected:
  Key dens_key_;
  Key pres_key_;
  Key gravity_key_;
  Key patm_key_;

};

} //namespace
} //namespace
} //namespace

#endif
