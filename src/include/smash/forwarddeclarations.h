/*
 *
 *    Copyright (c) 2014-2020
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_FORWARDDECLARATIONS_H_
#define SRC_INCLUDE_FORWARDDECLARATIONS_H_

/// @cond
// exclude most content here from documentation

#include <iosfwd>
#include <memory>
#include <vector>

#ifdef _LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_NAMESPACE_STD
#else
namespace std {
#endif

template <typename T>
class allocator;
template <typename T, typename A>
class vector;

template <typename T>
struct default_delete;
template <typename T, typename Deleter>
class unique_ptr;

template <std::size_t N>
class bitset;

#ifdef _LIBCPP_END_NAMESPACE_STD
_LIBCPP_END_NAMESPACE_STD
#else
}  // namespace std
#endif

namespace boost {
namespace filesystem {
class path;
}  // namespace filesystem
}  // namespace boost

namespace smash {

template <typename T>
using build_unique_ptr_ = std::unique_ptr<T, std::default_delete<T>>;
template <typename T>
using build_vector_ = std::vector<T, std::allocator<T>>;

class Action;
class ScatterAction;
class ScatterActionMulti;
class BoxModus;
class Clock;
class Configuration;
class CrossSections;
class DecayModes;
class DecayType;
class FourVector;
class ThreeVector;
class ModusDefault;
class OutputInterface;
class ParticleData;
class Particles;
class ParticleType;
class ParticleTypePtr;
class IsoParticleType;
class PdgCode;
class DecayBranch;
class CollisionBranch;
class Tabulation;
class ExperimentBase;
struct ExperimentParameters;
struct Nucleoncorr;

/// @endcond

/// The calculation frame
enum class CalculationFrame {
  CenterOfVelocity,
  CenterOfMass,
  FixedTarget,
};

/// Option to use Fermi Motion
enum class FermiMotion {
  /// Don't use fermi motion.
  Off,
  /// Use fermi motion in combination with potentials.
  On,
  /// Use fermi motion without potentials.
  Frozen,
};

/// Possible methods of impact parameter sampling.
enum class Sampling {
  /// Sample from uniform distribution.
  Uniform,
  /// Sample from areal / quadratic distribution.
  Quadratic,
  /// Sample from custom, user-defined distribution.
  Custom,
};

/// The time step mode.
enum class TimeStepMode : char {
  /// Don't use time steps; propagate from action to action.
  None,
  /// Use fixed time step.
  Fixed,
};

/**
 * Initial condition for a particle in a box.
 *
 * If PeakedMomenta is used, all particles have the same momentum
 * \f$p = 3 \cdot T\f$ with T being the temperature.
 *
 * Else, a thermalized ensemble is generated (the momenta are sampled
 * from a Maxwell-Boltzmann distribution).
 *
 * In either case, the positions in space are chosen randomly.
 */
enum class BoxInitialCondition {
  ThermalMomenta,
  PeakedMomenta,
};

/**
 * Initial condition for a particle in a sphere
 *
 * IC_ES, IC_1M and IC_2M are off-equilibrium distributions used in massless
 * comparisons of SMASH to the extended universe metric. They are described in
 * some detail in iref \iref{Bazow:2016oky}
 *
 * IC_Massive is a generalization of IC_ES for the non-zero mass case; note that
 * there is currently no analytical comparison possible with this distribution.
 *
 * The default value, ThermalMomenta, samples momenta from a Maxwell-Boltzmann
 * distribution and thus generates a thermal ensemble.
 */
enum class SphereInitialCondition {
  ThermalMomenta,
  IC_ES,
  IC_1M,
  IC_2M,
  IC_Massive,
};

/**
 * Defines properties of expansion for the metric (e.g. FRW)
 *
 * If anything else than NoExpansion is used, then a non-zero
 * Hubble parameter is computed and corrections are brought to the
 * propagation of all particles according to selected expanding
 * metric.
 */
enum class ExpansionMode {
  NoExpansion,
  MasslessFRW,
  MassiveFRW,
  Exponential,
};

/// Treatment of N Nbar Annihilation
enum class NNbarTreatment {
  /// No Annihilation
  NoAnnihilation,
  /// Use intermediate Resonances
  Resonances,
  /// Use string fragmentation
  Strings,
};

/// Represents thermodynamic quantities that can be printed out
enum class ThermodynamicQuantity : char {
  EckartDensity,
  Tmn,
  TmnLandau,
  LandauVelocity,
  j_QBS
};

/// Criteria used to check collisions
enum class CollisionCriterion {
  /// (Default) geometric criterion.
  Geometric,
  /// Stochastic Criteiron.
  Stochastic,
  /// Covariant Criterion
  Covariant
};

/// Whether and when only final state particles should be printed.
enum class OutputOnlyFinal {
  /// Print only final-state particles.
  Yes,
  /// Print initial, intermediate and final-state particles.
  No,
  /// Print only final-state particles, and those only if the event is not
  /// empty.
  IfNotEmpty,
};

/// The different groups of 2 to 2 reactions that one can include
enum IncludedReactions {
  All = 50,
  Elastic = 0,
  NN_to_NR = 1,
  NN_to_DR = 2,
  KN_to_KN = 3,
  KN_to_KDelta = 4,
  Strangeness_exchange = 5,
  NNbar = 6,
  PiDeuteron_to_NN = 7,
  PiDeuteron_to_pidprime = 8,
  NDeuteron_to_Ndprime = 9,
};

/// Container for the 2 to 2 reactions in the code
typedef std::bitset<10> ReactionsBitSet;

/**
 * Defines the algorithm used for the forced thermalization.
 *  For the description of algorithms see \iref{Oliinychenko:2016vkg}.
 *  All of them intend to conserve the net baryon number, strangeness
 *  and electric charge, as well as energy. Mode sampling is the fastest,
 *  but least theoretically robust, unbiased BF is the slowest
 *  (even hangs completely from time to time), but it is also the most
 *  theoretically robust.
 */
enum class ThermalizationAlgorithm {
  ModeSampling,
  BiasedBF,
  UnbiasedBF,
};

/// @cond
using ActionPtr = build_unique_ptr_<Action>;
using ScatterActionPtr = build_unique_ptr_<ScatterAction>;
using ScatterActionMultiPtr = build_unique_ptr_<ScatterActionMulti>;
using ActionList = build_vector_<ActionPtr>;

using OutputPtr = build_unique_ptr_<OutputInterface>;
using OutputsList = build_vector_<OutputPtr>;

using ParticleList = build_vector_<ParticleData>;
using ParticleTypeList = build_vector_<ParticleType>;
using ParticleTypePtrList = build_vector_<ParticleTypePtr>;
using IsoParticleTypeList = build_vector_<IsoParticleType>;

template <typename T>
using ProcessBranchPtr = build_unique_ptr_<T>;
template <typename T>
using ProcessBranchList = build_vector_<ProcessBranchPtr<T>>;
using DecayBranchPtr = build_unique_ptr_<DecayBranch>;
using DecayBranchList = build_vector_<DecayBranchPtr>;
using CollisionBranchPtr = build_unique_ptr_<CollisionBranch>;
using CollisionBranchList = build_vector_<CollisionBranchPtr>;

using TabulationPtr = build_unique_ptr_<Tabulation>;
using ExperimentPtr = build_unique_ptr_<ExperimentBase>;
using DecayTypePtr = build_unique_ptr_<DecayType>;

namespace bf = boost::filesystem;
/// @endcond

}  // namespace smash

#endif  // SRC_INCLUDE_FORWARDDECLARATIONS_H_
