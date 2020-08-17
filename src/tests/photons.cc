/*
 *    Copyright (c) 2016-2020
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */

#include <vir/test.h>  // This include has to be first

#include "setup.h"

#include "../include/smash/bremsstrahlungaction.h"
#include "../include/smash/scatteractionphoton.h"

using namespace smash;
using smash::Test::Momentum;

TEST(init_particle_types) {
  // enable debugging output
  create_all_loggers(Configuration(""));
  Test::create_actual_particletypes();
}

TEST(init_decay_modes) { Test::create_actual_decaymodes(); }

////
// Test photon production in binary scatterings
////

TEST(pi_rho0_pi_gamma) {
  // set up a π+ and a ρ0 in center-of-momentum-frame
  const ParticleType &type_pi = ParticleType::find(0x211);
  ParticleData pi{type_pi};
  pi.set_4momentum(type_pi.mass(),  // pole mass
                   ThreeVector(0., 0., 2.));
  const ParticleType &type_rho0 = ParticleType::find(0x113);
  ParticleData rho0{type_rho0};
  rho0.set_4momentum(type_rho0.mass(),  // pole mass
                     ThreeVector(0., 0., -2.));
  const int number_of_photons = 10000;
  ParticleList in{pi, rho0};
  const auto act =
      make_unique<ScatterActionPhoton>(in, 0.05, number_of_photons, 5.0);
  act->add_single_process();
  double tot_weight = 0.0;
  for (int i = 0; i < number_of_photons; i++) {
    act->generate_final_state();
    tot_weight += act->get_total_weight();
  }
  COMPARE_RELATIVE_ERROR(tot_weight, 0.000722419008, 0.08);
}

TEST(photon_reaction_type_function) {
  const ParticleData pip{ParticleType::find(0x211)};
  const ParticleData pim{ParticleType::find(-0x211)};
  const ParticleData piz{ParticleType::find(0x111)};
  const ParticleData rhop{ParticleType::find(0x213)};
  const ParticleData rhom{ParticleType::find(-0x213)};
  const ParticleData rhoz{ParticleType::find(0x113)};
  const ParticleData eta{ParticleType::find(0x221)};
  const ParticleData p{ParticleType::find(0x2112)};

  const ParticleList l1{pip, pim}, l2{rhop, pim}, l3{p, pim}, l4{pip, eta};

  VERIFY(ScatterActionPhoton::photon_reaction_type(l1) !=
         ScatterActionPhoton::ReactionType::no_reaction);
  VERIFY(ScatterActionPhoton::photon_reaction_type(l2) !=
         ScatterActionPhoton::ReactionType::no_reaction);
  VERIFY(ScatterActionPhoton::photon_reaction_type(l3) ==
         ScatterActionPhoton::ReactionType::no_reaction);
  VERIFY(ScatterActionPhoton::photon_reaction_type(l4) ==
         ScatterActionPhoton::ReactionType::no_reaction);
}

TEST(check_kinematic_thresholds) {
  /*
   * Make sure pi + pi -> rho + photon process is only executed if sqrt(s) is
   * high enough to not only create final state rho, but also to assign momentum
   * to rho and photon.
   */

  Particles particles;
  ParticleData a{ParticleType::find(0x211)};   // pi+
  ParticleData b{ParticleType::find(-0x211)};  // pi-
  ParticleData c{ParticleType::find(-0x211)};  // pi-

  /*
   * Pick energies such that energy_a + energy_b > m_rho_min + really_small
   * and energy_a + energy_c < m_rho_min + really_small.
   * Hence a+b should be performed while a+c should be rejected.
   */

  double energy_a = sqrt(pion_mass * pion_mass + 0.001 * 0.001);
  double energy_b = sqrt(pion_mass * pion_mass + 0.5 * 0.5);
  double energy_c = sqrt(pion_mass * pion_mass + 0.002 * 0.002);

  a.set_4momentum(Momentum{energy_a, 0.001, 0., 0.});
  b.set_4momentum(Momentum{energy_b, -0.5, 0., 0.});
  c.set_4momentum(Momentum{energy_c, -0.002, 0., 0.});

  a = particles.insert(a);
  b = particles.insert(b);
  c = particles.insert(c);

  // create underlying hadronic interactions
  constexpr double time = 1.;
  ScatterAction act_highE(a, b, time);
  ScatterAction act_lowE(a, c, time);

  // create photon scatter action
  ParticleList in_highE = act_highE.incoming_particles();
  ParticleList in_lowE = act_lowE.incoming_particles();
  ScatterActionPhoton photonAct_highE(in_highE, time, 1, 30.0);
  ScatterActionPhoton photonAct_lowE(in_lowE, time, 1, 30.0);

  VERIFY(
      photonAct_highE.is_kinematically_possible(energy_a + energy_b, in_highE));
  VERIFY(
      !photonAct_lowE.is_kinematically_possible(energy_a + energy_c, in_lowE));
}

////
// Test photon production in Bremsstrahlung processes
////

TEST(gen_final_state) {
  // set up a π+ and a π- in center-of-momentum-frame
  const ParticleType &type_pip = ParticleType::find(0x211);
  ParticleData pip{type_pip};
  pip.set_4momentum(type_pip.mass(), ThreeVector(0., 0., 2.));
  const ParticleType &type_pim = ParticleType::find(-0x211);
  ParticleData pim{type_pim};
  pim.set_4momentum(type_pim.mass(), ThreeVector(0., 0., -2.));
  const ParticleType &type_photon = ParticleType::find(0x22);
  const int number_of_photons = 10;
  ParticleList in{pip, pim};

  // create bremsstrahlung action
  const auto act =
      make_unique<BremsstrahlungAction>(in, 0.05, number_of_photons, 20.0);
  act->add_single_process();

  // Sample photons, implicitly test sample_3body_phasespace() and
  // cross section functions
  double tot_weight = 0.0;
  for (int i = 0; i < number_of_photons; i++) {
    act->generate_final_state();
    tot_weight += act->get_total_weight();
    VERIFY(act->outgoing_particles().size() == 3);
    VERIFY(act->outgoing_particles()[0].type() == type_pip);
    VERIFY(act->outgoing_particles()[1].type() == type_pim);
    VERIFY(act->outgoing_particles()[2].type() == type_photon);
  }
  COMPARE_RELATIVE_ERROR(tot_weight, 1.84592, 1e-5);
}

TEST(bremsstrahlung_reaction_type_function) {
  const ParticleData pip{ParticleType::find(0x211)};
  const ParticleData pim{ParticleType::find(-0x211)};
  const ParticleData piz{ParticleType::find(0x111)};
  const ParticleData eta{ParticleType::find(0x221)};
  const ParticleData p{ParticleType::find(0x2112)};

  const ParticleList l1{pip, pim}, l2{piz, pim}, l3{pip, pip}, l4{piz, piz},
      l5{pim, pim}, l6{pip, piz}, l7{p, pim}, l8{pip, eta};

  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l1) ==
         BremsstrahlungAction::ReactionType::pi_p_pi_m);
  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l2) ==
         BremsstrahlungAction::ReactionType::pi_z_pi_m);
  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l3) ==
         BremsstrahlungAction::ReactionType::pi_p_pi_p);
  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l4) ==
         BremsstrahlungAction::ReactionType::pi_z_pi_z);
  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l5) ==
         BremsstrahlungAction::ReactionType::pi_m_pi_m);
  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l6) ==
         BremsstrahlungAction::ReactionType::pi_z_pi_p);
  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l7) ==
         BremsstrahlungAction::ReactionType::no_reaction);
  VERIFY(BremsstrahlungAction::bremsstrahlung_reaction_type(l8) ==
         BremsstrahlungAction::ReactionType::no_reaction);
}
