/*
 *
 *    Copyright (c) 2013-2021
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "smash/angles.h"
#include "smash/chemicalpotential.h"
#include "smash/configuration.h"
#include "smash/constants.h"
#include "smash/experimentparameters.h"
#include "smash/fourvector.h"
#include "smash/hadgas_eos.h"
#include "smash/logging.h"
#include "smash/particles.h"
#include "smash/quantumsampling.h"
#include "smash/random.h"
#include "smash/spheremodus.h"
#include "smash/threevector.h"

namespace smash {
static constexpr int LSphere = LogArea::Sphere::id;

/*!\Userguide
 * \page input_modi_sphere_ Sphere
 *
 * \key Radius (double, required): \n
 * Radius of the Sphere, in fm.
 *
 * \key Temperature (double, required):\n
 * Temperature to sample momenta in the sphere, in GeV.
 *
 * \key Start_Time (double, required):\n
 * Starting time of sphere calculation.
 *
 * \key Init_Multiplicities
 * (int int, required if Use_Thermal_Multiplicities is false):\n
 * Initial multiplicities per particle species.
 * Map of PDG number and quantity of this PDG number.
 * Controls how many particles of each species will be initialized.
 *
 * \key Use_Thermal_Multiplicities (bool, optional, default = false): \n
 * If this option is set to true then Init_Multiplicities are ignored and the
 * box is initialized with all particle species of the particle table that
 * belong to the hadron gas equation of state (see
 * HadronGasEos::is_eos_particle()). The multiplicities are sampled from
 * Poisson distributions \f$ Poi(n_i V) \f$, where \f$ n_i \f$ are the
 * grand-canonical thermal densities of the corresponding species and \f$ V \f$
 * is the box volume. This option simulates the grand-canonical ensemble, where
 * the number of particles is not fixed from event to event.
 *
 * \key Baryon_Chemical_Potential (double, optional, default = 0.0): \n
 * Baryon chemical potential \f$ \mu_B \f$ only used if
 * Use_Thermal_Multiplicities is true to compute thermal densities \f$ n_i \f$.
 *
 * \key Strange_Chemical_Potential (double, optional, default = 0.0): \n
 * Strangeness chemical potential \f$ \mu_S \f$ only used if
 * Use_Thermal_Multiplicities is true to compute thermal densities \f$ n_i \f$.
 *
 * \key Charge_Chemical_Potential (double, optional, default = 0.0): \n
 * Charge chemical potential \f$ \mu_Q \f$ only used if
 * Use_Thermal_Multiplicities is true to compute thermal densities \f$ n_i \f$.
 *
 * \key Account_Resonance_Widths (bool, optional, default = true): \n
 * In case of thermal initialization: true -- account for resonance
 * spectral functions, while computing multiplicities and sampling masses,
 * false -- simply use pole masses.
 *
 * \key Initial_Condition (string, optional, default = "thermal momenta") \n
 * Initial distribution to use for momenta of particles. Mainly used in the
 * expanding universe scenario, options are:
 * \li \key thermal momenta - equilibrium Boltzmann distribution
 * \li \key IC_ES - off-equilibrium distribution
 * \li \key IC_1M - off-equilibrium distribution
 * \li \key IC_2M - off-equilibrium distribution
 * \li \key IC_Massive - off-equilibrium distribution
 *
 * See \iref{Bazow:2016oky} and \iref{Tindall:2016try} for further explanations
 * about the different distribution functions.
 *
 * \key Jet: \n
 * This subset of config values is used to put a single high energy particle
 * (a "jet") in the center of the sphere, on an outbound trajectory along
 * the x axis; if no pdg is specified no jet is produced.
 *
 * \li \key Jet_PDG (int, optional):
 * The type of particle to be used as a jet, as given by its PDG code;
 * if none is provided no jet is initialized.
 *
 * \li \key Jet_Momentum (double, optional, default = 20.):
 * The initial momentum to give to the jet particle (in GeV)
 *
 * \n
 * Examples: Configuring a Sphere Simulation
 * --------------
 * The following example configures an expanding sphere with a radius of 5 fm
 * at a temperature of 200 MeV. The particles are initialized
 * with thermal momenta at a start time of 0 fm. The particle numbers at
 * initialization are 100 \f$ \pi^+ \f$, 100 \f$ \pi^0 \f$, 100 \f$ \pi^- \f$,
 * 50 protons and 50 neutrons.
 *
 *\verbatim
 Modi:
     Sphere:
         Radius: 5.0
         Temperature: 0.2
         Initial_Condition: "thermal momenta"
         Start_Time: 0.0
         Init_Multiplicities:
             211: 100
             111: 100
             -211: 100
             2212: 50
             2112: 50
 \endverbatim
 *
 * It is also possible to initialize a sphere based on
 * thermal multiplicities. This is done via
 *\verbatim
 Modi:
     Sphere:
         Radius: 10.0
         Temperature: 0.2
         Use_Thermal_Multiplicities: True
 \endverbatim
 *
 * If one wants to simulate a jet in the hadronic medium, this can be done
 * by using the following configuration setup:
 *\verbatim
 Modi:
     Sphere:
         Radius: 10.0
         Temperature: 0.2
         Use_Thermal_Multiplicities: True
         Jet:
             Jet_PDG: 211
             Jet_Momentum: 100.0
\endverbatim
 *
 * \n
 * \note
 * SMASH is shipped with an example configuration file to set up an expanding
 * sphere simulation initialized with predefined initial particle
 * multiplicities. This file is located in /input/sphere. To run SMASH
 * with the provided example configuration for the sphere, execute \n
 * \n
 * \verbatim
    ./smash -i INPUT_DIR/sphere/config.yaml
 \endverbatim
 * \n
 * Where 'INPUT_DIR' needs to be replaced by the path to the input directory
 * ('../input', if the build directory is located in the smash
 * folder).
 *
 */

SphereModus::SphereModus(Configuration modus_config,
                         const ExperimentParameters &)
    : radius_(modus_config.take({"Sphere", "Radius"})),
      sphere_temperature_(modus_config.take({"Sphere", "Temperature"})),
      start_time_(modus_config.take({"Sphere", "Start_Time"}, 0.)),
      use_thermal_(
          modus_config.take({"Sphere", "Use_Thermal_Multiplicities"}, false)),
      mub_(modus_config.take({"Sphere", "Baryon_Chemical_Potential"}, 0.)),
      mus_(modus_config.take({"Sphere", "Strange_Chemical_Potential"}, 0.)),
      muq_(modus_config.take({"Sphere", "Charge_Chemical_Potential"}, 0.)),
      account_for_resonance_widths_(
          modus_config.take({"Sphere", "Account_Resonance_Widths"}, true)),
      init_multipl_(use_thermal_
                        ? std::map<PdgCode, int>()
                        : modus_config.take({"Sphere", "Init_Multiplicities"})
                              .convert_for(init_multipl_)),
      init_distr_(
          modus_config.take({"Sphere", "Initial_Condition"},
                            SphereInitialCondition::ThermalMomentaBoltzmann)),
      radial_velocity_(
          modus_config.take({"Sphere", "Add_Radial_Velocity"}, -1.)),
      insert_jet_(modus_config.has_value({"Sphere", "Jet", "Jet_PDG"})),
      jet_pdg_(insert_jet_ ? modus_config.take({"Sphere", "Jet", "Jet_PDG"})
                                 .convert_for(jet_pdg_)
                           : pdg::p),  // dummy default; never used
      jet_mom_(modus_config.take({"Sphere", "Jet", "Jet_Momentum"}, 20.)) {}

/* console output on startup of sphere specific parameters */
std::ostream &operator<<(std::ostream &out, const SphereModus &m) {
  out << "-- Sphere Modus:\nRadius of the sphere: " << m.radius_ << " fm\n";
  if (m.use_thermal_) {
    out << "Thermal multiplicities (T = " << m.sphere_temperature_
        << " GeV, muB = " << m.mub_ << " GeV, muS = " << m.mus_
        << " GeV, muQ = " << m.muq_ << " GeV)\n";
  } else {
    for (const auto &p : m.init_multipl_) {
      ParticleTypePtr ptype = &ParticleType::find(p.first);
      out << ptype->name() << " initial multiplicity " << p.second << '\n';
    }
  }
  switch (m.init_distr_) {
    case SphereInitialCondition::ThermalMomentaBoltzmann:
      out << "Boltzmann momentum distribution with T = "
          << m.sphere_temperature_ << " GeV.\n";
      break;
    case SphereInitialCondition::ThermalMomentaQuantum:
      out << "Fermi/Bose momentum distribution with T = "
          << m.sphere_temperature_ << " GeV.\n";
      break;
    case SphereInitialCondition::IC_ES:
      out << "Sphere Initial Condition is IC_ES";
      break;
    case SphereInitialCondition::IC_1M:
      out << "Sphere Initial Condition is IC_1M";
      break;
    case SphereInitialCondition::IC_2M:
      out << "Sphere Initial Condition is IC_2M";
      break;
    case SphereInitialCondition::IC_Massive:
      out << "Sphere Initial Condition is IC_Massive";
      break;
  }
  if (m.insert_jet_) {
    ParticleTypePtr ptype = &ParticleType::find(m.jet_pdg_);
    out << "Adding a " << ptype->name() << " as a jet in the middle "
        << "of the sphere with " << m.jet_mom_ << " GeV initial momentum.\n";
  }
  return out;
}

/* initial_conditions - sets particle data for @particles */
double SphereModus::initial_conditions(Particles *particles,
                                       const ExperimentParameters &parameters) {
  FourVector momentum_total(0, 0, 0, 0);
  const double T = this->sphere_temperature_;
  const double V = 4.0 / 3.0 * M_PI * radius_ * radius_ * radius_;
  /* Create NUMBER OF PARTICLES according to configuration */
  if (use_thermal_) {
    if (average_multipl_.empty()) {
      for (const ParticleType &ptype : ParticleType::list_all()) {
        if (HadronGasEos::is_eos_particle(ptype)) {
          const double n = HadronGasEos::partial_density(
              ptype, T, mub_, mus_, muq_, account_for_resonance_widths_);
          average_multipl_[ptype.pdgcode()] = n * V * parameters.testparticles;
        }
      }
    }
    double nb_init = 0.0, ns_init = 0.0, nq_init = 0.0;
    for (const auto &mult : average_multipl_) {
      const int thermal_mult_int = random::poisson(mult.second);
      particles->create(thermal_mult_int, mult.first);
      nb_init += mult.second * mult.first.baryon_number();
      ns_init += mult.second * mult.first.strangeness();
      nq_init += mult.second * mult.first.charge();
      logg[LSphere].debug(mult.first, " initial multiplicity ",
                          thermal_mult_int);
    }
    logg[LSphere].info("Initial hadron gas baryon density ", nb_init);
    logg[LSphere].info("Initial hadron gas strange density ", ns_init);
    logg[LSphere].info("Initial hadron gas charge density ", nq_init);
  } else {
    for (const auto &p : init_multipl_) {
      particles->create(p.second * parameters.testparticles, p.first);
      logg[LSphere].debug("Particle ", p.first, " initial multiplicity ",
                          p.second);
    }
  }
  std::unique_ptr<QuantumSampling> quantum_sampling;
  if (this->init_distr_ == SphereInitialCondition::ThermalMomentaQuantum) {
    quantum_sampling = make_unique<QuantumSampling>(init_multipl_, V, T);
  }
  /* loop over particle data to fill in momentum and position information */
  for (ParticleData &data : *particles) {
    Angles phitheta;
    /* thermal momentum according Maxwell-Boltzmann distribution */
    double momentum_radial = 0.0, mass = data.pole_mass();
    /* assign momentum_radial according to requested distribution */
    switch (init_distr_) {
      case (SphereInitialCondition::IC_ES):
        momentum_radial = sample_momenta_IC_ES(T);
        break;
      case (SphereInitialCondition::IC_1M):
        momentum_radial = sample_momenta_1M_IC(T, mass);
        break;
      case (SphereInitialCondition::IC_2M):
        momentum_radial = sample_momenta_2M_IC(T, mass);
        break;
      case (SphereInitialCondition::IC_Massive):
        momentum_radial = sample_momenta_non_eq_mass(T, mass);
        break;
      case (SphereInitialCondition::ThermalMomentaBoltzmann):
      default:
        mass = (!account_for_resonance_widths_)
                   ? data.type().mass()
                   : HadronGasEos::sample_mass_thermal(data.type(), 1.0 / T);
        momentum_radial = sample_momenta_from_thermal(T, mass);
        break;
      case (SphereInitialCondition::ThermalMomentaQuantum):
        /*
         * **********************************************************************
         * Sampling the thermal momentum according Bose/Fermi/Boltzmann
         * distribution.
         * We take the pole mass as the mass.
         * **********************************************************************
         */
        mass = data.type().mass();
        momentum_radial = quantum_sampling->sample(data.pdgcode());
        break;
    }
    phitheta.distribute_isotropically();
    logg[LSphere].debug(data.type().name(), "(id ", data.id(),
                        ") radial momentum ", momentum_radial, ", direction",
                        phitheta);
    data.set_4momentum(mass, phitheta.threevec() * momentum_radial);
    momentum_total += data.momentum();
    /* uniform sampling in a sphere with radius r */
    double position_radial;
    position_radial = std::cbrt(random::canonical()) * radius_;
    Angles pos_phitheta;
    pos_phitheta.distribute_isotropically();
    data.set_4position(
        FourVector(start_time_, pos_phitheta.threevec() * position_radial));
    data.set_formation_time(start_time_);
  }

  /* boost in radial direction with an underlying velocity field of the form u_r
   * = u_0 * r / R */
  if (radial_velocity_ > 0.0) {
    if (radial_velocity_ > 1.0) {
      throw std::invalid_argument(
          "Additional velocity cannot be greater than 1!");
    }
    for (ParticleData &data : *particles) {
      double particle_radius = std::sqrt(data.position().sqr3());
      auto e_r = data.position().threevec() / particle_radius;
      auto radial_velocity =
          -1.0 * radial_velocity_ * e_r * particle_radius / radius_;
      data.set_4momentum(data.momentum().lorentz_boost(radial_velocity));
      momentum_total += data.momentum();
    }
  }

  /* Make total 3-momentum 0 */
  for (ParticleData &data : *particles) {
    data.set_4momentum(data.momentum().abs(),
                       data.momentum().threevec() -
                           momentum_total.threevec() / particles->size());
  }

  /* Add a single highly energetic particle in the center of the sphere (jet) */
  if (insert_jet_) {
    auto &jet_particle = particles->create(jet_pdg_);
    jet_particle.set_formation_time(start_time_);
    jet_particle.set_4position(FourVector(start_time_, 0., 0., 0.));
    jet_particle.set_4momentum(ParticleType::find(jet_pdg_).mass(),
                               ThreeVector(jet_mom_, 0., 0.));
  }

  /* Recalculate total momentum */
  momentum_total = FourVector(0, 0, 0, 0);
  for (ParticleData &data : *particles) {
    momentum_total += data.momentum();
    /* IC: debug checks */
    logg[LSphere].debug() << data;
  }
  /* allows to check energy conservation */
  logg[LSphere].debug() << "Sphere initial total 4-momentum [GeV]: "
                        << momentum_total;
  return start_time_;
}
}  // namespace smash
