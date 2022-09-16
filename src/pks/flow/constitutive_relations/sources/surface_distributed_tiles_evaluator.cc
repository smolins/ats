/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/*
  License: see $ATS_DIR/COPYRIGHT
  Author: 
*/

/*!

Requires the following dependencies:


*/

#include "Key.hh"
#include "surface_distributed_tiles_evaluator.hh"

namespace Amanzi {
namespace Flow {
namespace Relations {


SurfDistributedTilesRateEvaluator::SurfDistributedTilesRateEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariableFieldEvaluator(plist),
    compatibility_checked_(false)
{

  domain_ = Keys::getDomain(my_key_);

  //  subsurface_marks_key_ = Keys::readKey(plist, domain_, "tile marks", "tile_marks");
  surface_marks_key_ = Keys::readKey(plist, domain_, "catchments_id", "catchments_id");
  surf_len_key_ = Keys::readKey(plist, domain_, "catchments_frac", "catchments_frac");
  sources_key_ = plist.get<std::string>("accumulated source key", "subdomain_sources");
  coupling_domain_src_key_ = plist.get<std::string>("coupling domain source key", "water_source");
  
  num_ditches_ = plist.get<int>("number of ditches");
  implicit_ = plist.get<bool>("implicit drainage", true);
  
  dependencies_.insert(surface_marks_key_);
  dependencies_.insert(surf_len_key_);
  dependencies_.insert("surface-pressure");
 
  times_.resize(2);
  times_[0] = -1.0; times_[1] = -1.0;
}

// Required methods from SecondaryVariableFieldEvaluator
void
SurfDistributedTilesRateEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
                                                  const Teuchos::Ptr<CompositeVector> & result)
{

  double t0 = S->time();
  double dt = *S->GetScalarData("dt"); 


  S->GetFieldEvaluator(coupling_domain_src_key_)->HasFieldChanged(S.ptr(), my_key_);
    

  Teuchos::RCP<Field> src_field =  S->GetField(sources_key_, "state");
  if (!implicit_){
    if (abs(t0 - times_[0]) > 1e-12) {
      src_field->SwitchCopies(Key("default"), Key("prev_timestep"));
      times_[0] = t0;
    }
  }
      
  const auto& surf_marks = *S->GetFieldData(surface_marks_key_)->ViewComponent("cell", false);
  const auto& len_frac = *S->GetFieldData(surf_len_key_)->ViewComponent("cell", false);
  const auto& cv =
      *S->GetFieldData(Keys::getKey(domain_,"cell_volume"))->ViewComponent("cell",false);  
  auto& surf_src = *result->ViewComponent("cell",false);
  
  Teuchos::RCP<const Epetra_Vector> src_vec;
                                            

  if (implicit_){
    src_vec = src_field->GetConstantVectorData();
  }else{
    src_vec = src_field->GetCopy("prev_timestep")->GetConstantVectorData();
  }
  
  //std::cout << "Surface source\n"<< *src_vec<<"\n";

  double total = 0.0;
  double frac_len = 0.0;
  
  AmanziMesh::Entity_ID ncells = surf_marks.MyLength();
  for (AmanziMesh::Entity_ID c=0; c!=ncells; ++c) {
    if ((surf_marks[0][c] > 0) && (dt>1e-14)) {
      surf_src[0][c] = -(*src_vec)[surf_marks[0][c] - 1]*len_frac[0][c]/(cv[0][c]*dt) ;
      //surf_src[0][c] = 0.0;
      if (surf_marks[0][c]==1) frac_len += len_frac[0][c];
        
      total += -(*src_vec)[surf_marks[0][c]-1]*len_frac[0][c];
    }
  }
  // std::cout<< "Total source: "<<total<<" "<<frac_len<<"\n";
  // total = 0.0;
  // for (AmanziMesh::Entity_ID c=0; c!=ncells; ++c) {
  //   total += surf_src[0][c]*cv[0][c];
  // }
  // std::cout<<"Total source: field "<<my_key_<<" "<<total<<"\n";
}

// Required methods from SecondaryVariableFieldEvaluator
void
SurfDistributedTilesRateEvaluator::EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
                     Key wrt_key, const Teuchos::Ptr<CompositeVector>& result)
{

  result->PutScalar(0.0);
  
}

void
SurfDistributedTilesRateEvaluator::EnsureCompatibility(const Teuchos::Ptr<State>& S) {

  if (!S->HasField(sources_key_)){
    S->RequireConstantVector(sources_key_, num_ditches_);
    Teuchos::RCP<Field> field =  S->GetField(sources_key_, "state");
    Teuchos::RCP<Field_ConstantVector> cvfield =
      Teuchos::rcp_dynamic_cast<Field_ConstantVector>(field, true);
    cvfield->CreateData();

    field->GetConstantVectorData()->PutScalar(0.0);
    field->set_initialized();
    
    Key prev_step("prev_timestep");
    field->RequireCopy(prev_step);
  }


  SecondaryVariableFieldEvaluator::EnsureCompatibility(S);
  S->GetField(my_key_, my_key_)->set_io_vis();
  
}

void
SurfDistributedTilesRateEvaluator::InitializeFromPlist_() {

}



} //namespace
} //namespace
} //namespace

