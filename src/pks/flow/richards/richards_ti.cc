/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
A base two-phase, thermal Richard's equation with water vapor.

Authors: Ethan Coon (ATS version) (ecoon@lanl.gov)
*/

#include "Epetra_FECrsMatrix.h"
#include "EpetraExt_RowMatrixOut.h"
#include "boost/math/special_functions/fpclassify.hpp"
#include "Mesh_MSTK.hh"

#include "richards.hh"

namespace Amanzi {
namespace Flow {

#define DEBUG_FLAG 1
#define DEBUG_RES_FLAG 0

// Richards is a BDFFnBase
// -----------------------------------------------------------------------------
// computes the non-linear functional g = g(t,u,udot)
// -----------------------------------------------------------------------------
void Richards::fun(double t_old,
                   double t_new,
                   Teuchos::RCP<TreeVector> u_old,
                   Teuchos::RCP<TreeVector> u_new,
                   Teuchos::RCP<TreeVector> g) {
  niter_++;

  // VerboseObject stuff.
  Teuchos::OSTab tab = getOSTab();

  if (dynamic_mesh_) matrix_->CreateMFDmassMatrices(K_.ptr());

  double h = t_new - t_old;
  ASSERT(std::abs(S_inter_->time() - t_old) < 1.e-4*h);
  ASSERT(std::abs(S_next_->time() - t_new) < 1.e-4*h);

  Teuchos::RCP<CompositeVector> u = u_new->data();

#if DEBUG_FLAG
  AmanziMesh::Entity_ID_List fnums1,fnums0;
  std::vector<int> dirs;
  mesh_->cell_get_faces_and_dirs(c0_, &fnums0, &dirs);
  mesh_->cell_get_faces_and_dirs(c1_, &fnums1, &dirs);

  if (out_.get() && includesVerbLevel(verbosity_, Teuchos::VERB_HIGH, true)) {
    *out_ << std::setprecision(15);
    *out_ << "----------------------------------------------------------------" << std::endl;
    *out_ << "Residual calculation: T0 = " << t_old
          << " T1 = " << t_new << " H = " << h << std::endl;
    *out_ << "  p(" << c0_ << "): " << (*u)("cell",c0_);
    for (int n=0; n!=fnums0.size(); ++n) *out_ << ",  " << (*u)("face",fnums0[n]);
    *out_ << std::endl;
    *out_ << "  p(" << c1_ << "): " << (*u)("cell",c1_);
    for (int n=0; n!=fnums1.size(); ++n) *out_ << ",  " << (*u)("face",fnums1[n]);
    *out_ << std::endl;
  }
#endif

  // pointer-copy temperature into state and update any auxilary data
  solution_to_state(u_new, S_next_);

  // update boundary conditions
  bc_pressure_->Compute(t_new);
  bc_flux_->Compute(t_new);
  UpdateBoundaryConditions_();

  // zero out residual
  Teuchos::RCP<CompositeVector> res = g->data();
  res->PutScalar(0.0);

  // diffusion term, treated implicitly
  ApplyDiffusion_(S_next_.ptr(), res.ptr());

#if DEBUG_FLAG
  if (out_.get() && includesVerbLevel(verbosity_, Teuchos::VERB_HIGH, true)) {
    Teuchos::RCP<const CompositeVector> satl1 =
        S_next_->GetFieldData("saturation_liquid");
    Teuchos::RCP<const CompositeVector> satl0 =
        S_inter_->GetFieldData("saturation_liquid");


    Teuchos::RCP<const CompositeVector> relperm =
        S_next_->GetFieldData("relative_permeability");
    Teuchos::RCP<const Epetra_MultiVector> relperm_bf =
        relperm->ViewComponent("boundary_face",false);
    Teuchos::RCP<const CompositeVector> uw_relperm =
        S_next_->GetFieldData("numerical_rel_perm");
    Teuchos::RCP<const Epetra_MultiVector> uw_relperm_bf =
        uw_relperm->ViewComponent("boundary_face",false);
    Teuchos::RCP<const CompositeVector> flux_dir =
        S_next_->GetFieldData("darcy_flux_direction");

    if (S_next_->HasField("saturation_ice")) {
      Teuchos::RCP<const CompositeVector> sati1 =
          S_next_->GetFieldData("saturation_ice");
      Teuchos::RCP<const CompositeVector> sati0 =
          S_inter_->GetFieldData("saturation_ice");
      *out_ << "    sat_old(" << c0_ << "): " << (*satl0)("cell",c0_) << ", "
            << (*sati0)("cell",c0_) << std::endl;
      *out_ << "    sat_new(" << c0_ << "): " << (*satl1)("cell",c0_) << ", "
            << (*sati1)("cell",c0_) << std::endl;
      *out_ << "    sat_old(" << c1_ << "): " << (*satl0)("cell",c1_) << ", "
            << (*sati0)("cell",c1_) << std::endl;
      *out_ << "    sat_new(" << c1_ << "): " << (*satl1)("cell",c1_) << ", "
            << (*sati1)("cell",c1_) << std::endl;
    } else {
      *out_ << "    sat_old(" << c0_ << "): " << (*satl0)("cell",c0_) << std::endl;
      *out_ << "    sat_new(" << c0_ << "): " << (*satl1)("cell",c0_) << std::endl;
      *out_ << "    sat_old(" << c1_ << "): " << (*satl0)("cell",c1_) << std::endl;
      *out_ << "    sat_new(" << c1_ << "): " << (*satl1)("cell",c1_) << std::endl;
    }

    *out_ << "    k_rel(" << c0_ << "): " << (*relperm)("cell",c0_);
    for (int n=0; n!=fnums0.size(); ++n) *out_ << ",  " << (*uw_relperm)("face",fnums0[n]);
    *out_ << std::endl;
    *out_ << "    k_rel(" << c1_ << "): " << (*relperm)("cell",c1_);
    for (int n=0; n!=fnums1.size(); ++n) *out_ << ",  " << (*uw_relperm)("face",fnums1[n]);
    *out_ << std::endl;

    *out_ << "  res(" << c0_ << ") (after diffusion): " << (*res)("cell",c0_);
    for (int n=0; n!=fnums0.size(); ++n) *out_ << ",  " << (*res)("face",fnums0[n]);
    *out_ << std::endl;
    *out_ << "  res(" << c1_ << ") (after diffusion): " << (*res)("cell",c1_);
    for (int n=0; n!=fnums1.size(); ++n) *out_ << ",  " << (*res)("face",fnums1[n]);
    *out_ << std::endl;
  }
#endif

  // accumulation term
  AddAccumulation_(res.ptr());

#if DEBUG_FLAG
  if (out_.get() && includesVerbLevel(verbosity_, Teuchos::VERB_HIGH, true)) {
    *out_ << "  res(" << c0_ << ") (after accumulation): " << (*res)("cell",c0_)
          << " " << (*res)("face",fnums0[0]) << std::endl;
    *out_ << "  res(" << c1_ << ") (after accumulation): " << (*res)("cell",c1_)
          << " " << (*res)("face",fnums1[1]) << std::endl;
  }
#endif

#if DEBUG_RES_FLAG
  if (niter_ < 23) {
    std::stringstream namestream;
    namestream << "flow_residual_" << niter_;
    *S_next_->GetFieldData(namestream.str(),name_) = *res;

    std::stringstream solnstream;
    solnstream << "flow_solution_" << niter_;
    *S_next_->GetFieldData(solnstream.str(),name_) = *u;
  }
#endif
};

// -----------------------------------------------------------------------------
// Apply the preconditioner to u and return the result in Pu.
// -----------------------------------------------------------------------------
void Richards::precon(Teuchos::RCP<const TreeVector> u, Teuchos::RCP<TreeVector> Pu) {
  // VerboseObject stuff.
  Teuchos::OSTab tab = getOSTab();

#if DEBUG_FLAG
  AmanziMesh::Entity_ID_List fnums1,fnums0;
  std::vector<int> dirs;
  mesh_->cell_get_faces_and_dirs(c0_, &fnums0, &dirs);
  mesh_->cell_get_faces_and_dirs(c1_, &fnums1, &dirs);

  // Dump residual
  if (out_.get() && includesVerbLevel(verbosity_, Teuchos::VERB_HIGH, true)) {
    *out_ << "Precon application:" << std::endl;
    *out_ << "  p(" << c0_ << "): " << (*u->data())("cell",c0_) << " "
          << (*u->data())("face",fnums0[0]) << std::endl;
    *out_ << "  p(" << c1_ << "): " << (*u->data())("cell",c1_) << " "
          << (*u->data())("face",fnums1[1]) << std::endl;
  }
#endif

  // Apply the preconditioner
  preconditioner_->ApplyInverse(*u, Pu.ptr());

#if DEBUG_FLAG
  // Dump correction
  if (out_.get() && includesVerbLevel(verbosity_, Teuchos::VERB_HIGH, true)) {
    *out_ << "  PC*p(" << c0_ << "): " << (*Pu->data())("cell",c0_) << " "
          << (*Pu->data())("face",fnums0[0]) << std::endl;
    *out_ << "  PC*p(" << c1_ << "): " << (*Pu->data())("cell",c1_) << " "
          << (*Pu->data())("face",fnums1[1]) << std::endl;
  }
#endif
};


// -----------------------------------------------------------------------------
// Update the preconditioner at time t and u = up
// -----------------------------------------------------------------------------
void Richards::update_precon(double t, Teuchos::RCP<const TreeVector> up, double h) {
  // VerboseObject stuff.
  Teuchos::OSTab tab = getOSTab();

#if DEBUG_FLAG
  if (out_.get() && includesVerbLevel(verbosity_, Teuchos::VERB_HIGH, true)) {
    *out_ << "Precon update at t = " << t << std::endl;
  }
#endif
  
  if (dynamic_mesh_) {
    matrix_->CreateMFDmassMatrices(K_.ptr());
    mfd_preconditioner_->CreateMFDmassMatrices(K_.ptr());
  }

  // update state with the solution up.
  ASSERT(std::abs(S_next_->time() - t) <= 1.e-4*t);
  PKDefaultBase::solution_to_state(up, S_next_);

  // update the rel perm according to the scheme of choice
  UpdatePermeabilityData_(S_next_.ptr());

  // update boundary conditions
  bc_pressure_->Compute(S_next_->time());
  bc_flux_->Compute(S_next_->time());
  UpdateBoundaryConditions_();

  Teuchos::RCP<const CompositeVector> rel_perm =
      S_next_->GetFieldData("numerical_rel_perm");
  Teuchos::RCP<const CompositeVector> rho =
      S_next_->GetFieldData("mass_density_liquid");
  Teuchos::RCP<const Epetra_Vector> gvec =
      S_next_->GetConstantVectorData("gravity");

  // Update the preconditioner with darcy and gravity fluxes
  mfd_preconditioner_->CreateMFDstiffnessMatrices(rel_perm.ptr());
  mfd_preconditioner_->CreateMFDrhsVectors();
  AddGravityFluxes_(gvec.ptr(), rel_perm.ptr(), rho.ptr(), mfd_preconditioner_.ptr());

  // Update the preconditioner with accumulation terms.
  // -- update the accumulation derivatives
  S_next_->GetFieldEvaluator("water_content")
      ->HasFieldDerivativeChanged(S_next_.ptr(), name_, key_);

  // -- get the accumulation deriv
  const Epetra_MultiVector& dwc_dp =
      *S_next_->GetFieldData("dwater_content_d"+key_)->ViewComponent("cell",false);
  const Epetra_MultiVector& pres =
      *S_next_->GetFieldData(key_)->ViewComponent("cell",false);

  // -- update the cell-cell block
  std::vector<double>& Acc_cells = mfd_preconditioner_->Acc_cells();
  std::vector<double>& Fc_cells = mfd_preconditioner_->Fc_cells();
  int ncells = dwc_dp.MyLength();
  for (int c=0; c!=ncells; ++c) {
    Acc_cells[c] += dwc_dp[0][c] / h;
    Fc_cells[c] += pres[0][c] * dwc_dp[0][c] / h;
  }

  // Assemble and precompute the Schur complement for inversion.
  mfd_preconditioner_->ApplyBoundaryConditions(bc_markers_, bc_values_);

  if (assemble_preconditioner_) {
    mfd_preconditioner_->AssembleGlobalMatrices();
    mfd_preconditioner_->ComputeSchurComplement(bc_markers_, bc_values_);
    mfd_preconditioner_->UpdatePreconditioner();
  }

  /*
  // dump the schur complement
  Teuchos::RCP<Epetra_FECrsMatrix> sc = mfd_preconditioner_->Schur();
  std::stringstream filename_s;
  filename_s << "schur_" << S_next_->cycle() << ".txt";
  EpetraExt::RowMatrixToMatlabFile(filename_s.str().c_str(), *sc);
  *out_ << "updated precon " << S_next_->cycle() << std::endl;

  // print the rel perm
  Teuchos::RCP<const CompositeVector> cell_rel_perm =
      S_next_->GetFieldData("relative_permeability");
  *out_ << "REL PERM: " << std::endl;
  cell_rel_perm->Print(*out_);
  *out_ << std::endl;
  *out_ << "UPWINDED REL PERM: " << std::endl;
  rel_perm->Print(*out_);
  */

};


void Richards::set_preconditioner(const Teuchos::RCP<Operators::Matrix> precon) {
  preconditioner_ = precon;
  mfd_preconditioner_ = Teuchos::rcp_dynamic_cast<Operators::MatrixMFD>(precon);
  ASSERT(mfd_preconditioner_ != Teuchos::null);
  mfd_preconditioner_->SetSymmetryProperty(symmetric_);
  mfd_preconditioner_->SymbolicAssembleGlobalMatrices();
  mfd_preconditioner_->InitPreconditioner();

}


double Richards::enorm(Teuchos::RCP<const TreeVector> u,
                       Teuchos::RCP<const TreeVector> du) {
  // Calculate water content at the solution.
  S_next_->GetFieldEvaluator("water_content")->HasFieldChanged(S_next_.ptr(), name_);
  const Epetra_MultiVector& wc = *S_next_->GetFieldData("water_content")
      ->ViewComponent("cell",false);
  const Epetra_MultiVector& flux = *S_next_->GetFieldData("darcy_flux")
      ->ViewComponent("face",false);
  double flux_max(0.);
  flux.NormInf(&flux_max);


  Teuchos::RCP<const CompositeVector> res = du->data();
  const Epetra_MultiVector& res_c = *res->ViewComponent("cell",false);
  const Epetra_MultiVector& res_f = *res->ViewComponent("face",false);
  const Epetra_MultiVector& pres_f = *u->data()->ViewComponent("face",false);
  double h = S_next_->time() - S_inter_->time();

  // Cell error is based upon error in mass conservation relative to
  // the current water content
  double enorm_cell(0.);
  int ncells = res_c.MyLength();
  for (int c=0; c!=ncells; ++c) {
    double tmp = std::abs(h*res_c[0][c]) / (atol_+rtol_*std::abs(wc[0][c]));
    enorm_cell = std::max<double>(enorm_cell, tmp);
  }

  // Face error is mismatch in flux, so relative to flux.
  double enorm_face(0.);
  int nfaces = res_f.MyLength();
  for (int f=0; f!=nfaces; ++f) {
    double tmp = 1.e-4 * std::abs(res_f[0][f]) / (atol_ + rtol_*flux_max);
    enorm_face = std::max<double>(enorm_face, tmp);
  }


  // Write out Inf norms too.
  Teuchos::OSTab tab = getOSTab();
  if (out_.get() && includesVerbLevel(verbosity_, Teuchos::VERB_MEDIUM, true)) {
    double infnorm_c(0.), infnorm_f(0.);
    res_c.NormInf(&infnorm_c);
    res_f.NormInf(&infnorm_f);

#ifdef HAVE_MPI
    double buf_c(enorm_cell), buf_f(enorm_face);
    MPI_Allreduce(&buf_c, &enorm_cell, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&buf_f, &enorm_face, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
#endif

    *out_ << "ENorm (cells) = " << enorm_cell << " (" << infnorm_c << ")  " << std::endl;
    *out_ << "ENorm (faces) = " << enorm_face << " (" << infnorm_f << ")  " << std::endl;
  }

  double enorm_val(std::max<double>(enorm_face, enorm_cell));
#ifdef HAVE_MPI
  double buf = enorm_val;
  MPI_Allreduce(&buf, &enorm_val, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
#endif
  return enorm_val;
};


}  // namespace Flow
}  // namespace Amanzi



