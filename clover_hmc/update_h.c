// -----------------------------------------------------------------
#include "cl_dyn_includes.h"
// -----------------------------------------------------------------



// -----------------------------------------------------------------
// Update the momenta
void update_anti_hermitian(site *s, int dir, Real eps, su3_matrix_f *force) {
  su3_matrix_f tmatf;

  uncompress_anti_hermitian(&(s->mom[dir]), &tmatf);
  scalar_mult_dif_su3_matrix_f(force, eps, &tmatf);
  make_anti_hermitian(&tmatf, &(s->mom[dir]));
}
// -----------------------------------------------------------------



// -----------------------------------------------------------------
// Update the momenta with the gauge force
double gauge_force(Real eps) {
  register int i, dir, dir2;
  register site *s;
  register Real eb3 = eps * beta / (Real)NCOL;
  double norm = 0.0;
  msg_tag *tag0, *tag1, *tag2;
  su3_matrix_f tmatf;

  // Loop over directions, update mom[dir]
  FORALLUPDIR(dir) {
    // Initialize fundamental staple
    FORALLSITES(i, s)
      clear_su3mat_f(&(staplef[i]));

    /* Loop over other directions, computing force from plaquettes in
       the dir, dir2 plane */
    FORALLUPDIR(dir2) {
      if (dir2 == dir)
        continue;

      /* get link[dir2] from direction dir */
      tag0 = start_gather_site(F_OFFSET(linkf[dir2]), sizeof(su3_matrix_f),
                               dir, EVENANDODD, gen_pt[0]);

      /* Start gather for the "upper staple" */
      tag2 = start_gather_site(F_OFFSET(linkf[dir]), sizeof(su3_matrix_f),
                               dir2, EVENANDODD, gen_pt[2]);

      /* begin the computation "at the dir2DOWN point", we will
         later gather the intermediate result "to the home point" */
      wait_gather(tag0);
      FORALLSITES(i, s) {
        mult_su3_an_f(&(s->linkf[dir2]), &(s->linkf[dir]), &tmatf);
        mult_su3_nn_f(&tmatf, (su3_matrix_f *)gen_pt[0][i], &(tempmatf[i]));
      }

      /* Gather lower staple "up to home site" */
      tag1 = start_gather_field(tempmatf, sizeof(su3_matrix_f),
                                OPP_DIR(dir2), EVENANDODD, gen_pt[1]);

      /*  The "upper" staple.  Note that
          one of the links has already been gathered, since it
          was used in computing the "lower" staple of the site
          above us (in dir2) */
      wait_gather(tag2);
      FORALLSITES(i, s) {
        mult_su3_nn_f(&(s->linkf[dir2]), (su3_matrix_f *)gen_pt[2][i],
                      &tmatf);
        mult_su3_na_sum_f(&tmatf, (su3_matrix_f *)gen_pt[0][i], &(staplef[i]));
      }

      wait_gather(tag1);
      FORALLSITES(i, s)
        sum_su3_matrix_f((su3_matrix_f *)gen_pt[1][i], &(staplef[i]));

      cleanup_gather(tag0);
      cleanup_gather(tag1);
      cleanup_gather(tag2);
    }

    // Now multiply the staple sum by the link, then update momentum
    FORALLSITES(i, s) {
      mult_su3_na_f(&(s->linkf[dir]), &(staplef[i]), &tmatf);
      update_anti_hermitian(s, dir, eb3, &tmatf);
      norm += (double)realtrace_su3_f(&tmatf, &tmatf);
    }
  }
  g_doublesum(&norm);
  return (eb3 * sqrt(norm) / (double)volume);
}
// -----------------------------------------------------------------



// -----------------------------------------------------------------
// Fermion force from hopping and clover terms
// Expects that the vectors psi and mp have been prepared by prepare_vecs
void delta(int level) {
  register int i, mu, nu;
  register site *s;
  Real tCKU0 = -CKU0 / 8.0;
  su3_matrix tmat;
  half_wilson_vector thvec;
  wilson_vector twvec, twvec2;
  msg_tag *tag0, *tag1;

  FORALLUPDIR(mu) {
#ifdef CG_DEBUG
    node0_printf("mu = %d\n", mu);
#endif

    FORALLSITES(i, s) {
      wp_shrink(&(psi[level][i]), &(htmp[0][i]), mu, PLUS);
      wp_shrink(&(mp[i]), &(htmp[1][i]), mu, MINUS);
    }
    tag0 = start_gather_field(htmp[0], sizeof(half_wilson_vector),
                              mu, EVENANDODD, gen_pt[0]);
    tag1 = start_gather_field(htmp[1], sizeof(half_wilson_vector),
                              mu, EVENANDODD, gen_pt[1]);
    wait_gather(tag0);
    wait_gather(tag1);

    /* First the U d(dslash)/dU terms (do for only one value of nu) */
    FORALLSITES(i, s) {
      /* psi and mp parallel transported in from positive directions */
      mult_su3_mat_hwvec(&(s->link[mu]), (half_wilson_vector *)gen_pt[0][i],
                         &thvec);
      wp_grow(&thvec, &twvec, mu, PLUS);
      /* i even => twvec = (1+gamma_mu)*U*Aodd^(-1)*D*psi,
         i odd  => twvec = (1+gamma_mu)*U*psi */

      mult_su3_mat_hwvec(&(s->link[mu]), (half_wilson_vector *)gen_pt[1][i],
                         &thvec);
      wp_grow(&thvec, &twvec2, mu, MINUS);
      /* i even => twvec2 = (1-gamma_mu)*U*Aodd^(-1)*D_adj*M*psi,
         i odd  => twvec2 = (1-gamma_mu)*U*M*psi */

      su3_projector_w(&twvec, &(mp[i]), &(tempmat[i]));
      su3_projector_w(&twvec2, &(psi[level][i]), &tmat);
      sum_su3_matrix(&tmat, &(tempmat[i]));
      scalar_mult_su3_matrix(&(tempmat[i]), mkappa, &(tempmat[i]));
      sum_su3_matrix(&(tempmat[i]), &(s->Force[mu]));
    }
    cleanup_gather(tag0);
    cleanup_gather(tag1);

    // Now the U dA/dU terms
    FORALLUPDIR(nu) {
      if (nu == mu)
        continue;

      // U dA/dU from U dM/dU
      udadu_mu_nu(mp, psi[level], mu, nu);       // Result in tempmat
      FORALLSITES(i, s)
        scalar_mult_sum_su3_matrix(&(tempmat[i]), tCKU0, &(s->Force[mu]));

      // U dA/dU from U dM^dagger/dU
      udadu_mu_nu(psi[level], mp, mu, nu);       // Result in tempmat
      FORALLSITES(i, s)
        scalar_mult_sum_su3_matrix(&(tempmat[i]), tCKU0, &(s->Force[mu]));
    }
  }
}
// -----------------------------------------------------------------



// -----------------------------------------------------------------
// Gauge force from fermion irrep plaquette term
void gauge_force_frep(int dir) {
  register int i, dir2;
  register site *s;
  Real bfrep_force;
  msg_tag *tag0, *tag1, *tag2;
  su3_matrix tmat;

  // Divide by nflavors to compensate for ferm_epsilon = nflavors * eps
  bfrep_force = beta_frep / (Real)(DIMF * nflavors);

  /* Loop over directions for updating mom[dir] is in the calling routine.
     Here, we loop over other directions, computing force from frep plaquettes
     in the dir, dir2 plane
     */
  FORALLUPDIR(dir2) {
    if (dir2 == dir)
      continue;
    /* get link[dir2] from direction dir */
    tag0 = start_gather_site(F_OFFSET(link[dir2]), sizeof(su3_matrix),
                             dir, EVENANDODD, gen_pt[0]);

    /* Start gather for the "upper staple" */
    tag2 = start_gather_site(F_OFFSET(link[dir]), sizeof(su3_matrix),
                             dir2, EVENANDODD, gen_pt[2]);

    /* begin the computation "at the dir2DOWN point", we will
       later gather the intermediate result "to the home point" */
    // Initialize tempmat2
    wait_gather(tag0);
    FORALLSITES(i, s) {
      mult_su3_an(&(s->link[dir2]), &(s->link[dir]), &tmat);
      mult_su3_nn(&tmat, (su3_matrix *)gen_pt[0][i], &(tempmat2[i]));
    }

    /* Gather lower staple "up to home site" */
    tag1 = start_gather_field(tempmat2, sizeof(su3_matrix),
                             OPP_DIR(dir2), EVENANDODD, gen_pt[1]);

    /*  The "upper" staple.  Note that
        one of the links has already been gathered, since it
        was used in computing the "lower" staple of the site
        above us (in dir2) */
    wait_gather(tag2);
    FORALLSITES(i, s) {
      mult_su3_nn(&(s->link[dir2]), (su3_matrix *)gen_pt[2][i], &tmat);
      mult_su3_na_sum(&tmat, (su3_matrix *)gen_pt[0][i], &(staple[i]));
    }

    // Initialize staple
    wait_gather(tag1);
    FORALLSITES(i, s)
      sum_su3_matrix((su3_matrix *)gen_pt[1][i], &(staple[i]));

    cleanup_gather(tag0);
    cleanup_gather(tag1);
    cleanup_gather(tag2);
  }

  // Now multiply the staple sum by the link, then add to force
  FORALLSITES(i, s) {
    mult_su3_na(&(s->link[dir]), &(staple[i]), &tmat);
    scalar_mult_sum_su3_matrix(&tmat, bfrep_force, &(s->Force[dir]));
  }
}
// -----------------------------------------------------------------



// -----------------------------------------------------------------
// Prepare vectors psi[level] and mp for delta:
// psi[level]_o <-- kappa * R_o^(-1) * dslash_oe * psi[level]_e
// psi[level]_e <-- psi[level]_e
// mp_e <-- M.psi[level]_e
// mp_o <-- kappa * R_o^(-1) * dslash_oe^dag * M.psi[level]_e
// Somewhat hacky: Positive MSq hits mp on all sites
void prepare_vecs(int level, Real MSq) {
  register int i;
  register site *s;
  wilson_vector twvec, twvec2;

  // fermion_op computes the temporary p_o <-- R_o^(-1) * dslash_oe * psi_e
  fermion_op(psi[level], mp, PLUS, EVEN);
  FORODDSITES(i, s)
    scalar_mult_wvec(&(p[i]), kappa, &(psi[level][i]));

  if (level == 1) {
    FOREVENSITES(i, s) {
      // Add i * shift * gamma5.psi[level] to M.psi[level]
      mult_by_gamma(&(psi[level][i]), &twvec, GAMMAFIVE);
      c_scalar_mult_sum_wvec(&twvec, &ishift, &(mp[i]));
    }
  }

  dslash(mp, tempwvec, MINUS, ODD);   // dslash_oe^dag * M.psi[level]_e
  mult_ldu(tempwvec, mp, ODD);
  FORODDSITES(i, s)
    scalar_mult_wvec(&(mp[i]), kappa, &(mp[i]));

  if (MSq > IMAG_TOL) {
    FORALLSITES(i, s)
      scalar_mult_wvec(&(mp[i]), MSq, &(mp[i]));
  }
}
// -----------------------------------------------------------------




// -----------------------------------------------------------------
// Update the  momenta with the fermion force
/* Assumes that the conjugate gradient has been run, with the answer in psi */
/* pseudofermion vector is chi */

/* Use the LU preconditioned fermion matrix, where
  the fermion spinors live on even sites only.  In other words, if
  Dslash_oe is the dslash operator with its source on even sites and
  its result on odd sites, etc.:
    M = A_e - kappa^2 * Dslash_eo (A_o)^{-1}* Dslash_oe
*/
double fermion_force(Real eps1, Real eps2) {
  register int i, mu, nu;
  register site *s;
  int level;
  Real tCKU0 = -CKU0 / 4.0, ferm_epsilon = nflavors * eps1;
  Real MSq = shift * shift * eps2 / eps1;
  double maxnorm = 0.0, norm = 0.0, tr;
  su3_matrix_f tmatf;
  su3_matrix tmat;
#ifdef DEBUG_CHECK
  double dtime = -dclock(), mflops;
#endif

#ifdef TIMING
TIC(1)
#endif

  // Clear the force collectors
  FORALLUPDIR(mu) {
    FORALLSITES(i, s)
      clear_su3mat(&(s->Force[mu]));
  }

  /* while we're at it, get the diagonal clover entry */
  /* With A_odd == sigma_{\mu\nu} F_{\mu\nu}, now add force term
     U dF_{\mu\nu}/dU Tr_{dirac} (sigma_{\mu\nu} A_odd^{-1}) */
  FORALLUPDIR(mu) {
    FORALLUPDIR(nu) {
      if (nu == mu)
        continue;
      tr_sigma_ldu_mu_nu(mu, nu);         // Result in tempmat
      udadu_mat_mu_nu(mu, nu);            // Result in tempmat2
      FORALLSITES(i, s)
        scalar_mult_sum_su3_matrix(&(tempmat2[i]), tCKU0, &(s->Force[mu]));
    }
  }

  if (fabs(beta_frep) > IMAG_TOL) {
    FORALLUPDIR(mu)
      gauge_force_frep(mu);
  }

  if (num_masses == 1)
    level = 0;
  else
    level = 1;
  prepare_vecs(level, 0.0);
  delta(level);

  if (eps2 > IMAG_TOL && num_masses == 2) {
    prepare_vecs(0, MSq);
    delta(0);
  }

  FORALLUPDIR(mu) {
    FORALLSITES(i, s) {
      /* back from fermion irrep to fundamental links
         compatible with fat links
         W = link in fermion irrep (can be thin or fat)
         V = fat fundamental link
         U = thin fundamental link
         */
      /* remove W from W*dH/dW^T      */
      mult_su3_an(&(s->link[mu]), &(s->Force[mu]), &tmat);

      /* dH/dV^T from dW/dV^T dH/dW^T */
      chain_rule(&tmatf, &tmat, &(gauge_field[mu][i]));

      // Take care of boundary conditions
      apply_bc(&tmatf, mu, s->t);

      // Not done yet.  For now, save dH/dV^T
      su3mat_copy_f(&tmatf, &(Sigma[mu][i]));
    }
  }

  // More chain rule: dH/dU^T = dV/dU^T dH/dV^T
  // nhyp_force receives dH/dV^T and returns dH/dU^T both in Sigma[mu]
  nhyp_force1();
  // Finally we can update
  FORALLUPDIR(mu) {
    FORALLSITES(i, s) {
      // First multiply back U.dH/dU^T
      mult_su3_nn_f(&(s->linkf[mu]), &(Sigma[mu][i]), &tmatf);
      // Now update
      update_anti_hermitian(s, mu, ferm_epsilon, &tmatf);
      tr = (double)realtrace_su3_f(&tmatf, &tmatf);
      norm += tr;
      if (tr > maxnorm)
        maxnorm = tr;
    }
  }
  g_doublesum(&norm);
#ifdef TIMING
  TOC(1, time_fermion_force)
#endif

#ifdef DEBUG_CHECK
  dtime += dclock();
  mflops = (double)(5584.0 * volume / (1e6 * dtime * numnodes()));
  node0_printf("Fermion force time = %.4g mflops = %.4g\n",
               dtime, mflops);
#endif
  return (2.0 * ferm_epsilon * sqrt(norm) / (double)volume);
}
// -----------------------------------------------------------------
