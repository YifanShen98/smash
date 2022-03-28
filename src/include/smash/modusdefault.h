/*
 *
 *    Copyright (c) 2013-2022
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SMASH_MODUSDEFAULT_H_
#define SRC_INCLUDE_SMASH_MODUSDEFAULT_H_

#include <memory>

#include "configuration.h"
#include "cxx14compat.h"
#include "forwarddeclarations.h"
#include "fourvector.h"
#include "grandcan_thermalizer.h"
#include "grid.h"
#include "outputinterface.h"
#include "potentials.h"

namespace smash {

/**
 * \ingroup modus
 * Base class for Modus classes that provides default function implementations.
 *
 * This is only a base class for actual Modus classes. Meaning there will never
 * be objects, references, or pointers to ModusDefault. Therefore, it does not
 * have - and will never need any virtual functions.
 *
 * The rules for adding functions to this class are as follows:
 * - This class is empty per default.
 * - You can add a function if you have a function that is different in at least
 *   one subclass.
 * - Code that is common to all goes into ExperimentImplementation.
 *
 * \todo JB: many of these functions could/should be virtual (contradicts
 * description given above, Vinzent says (in a nice way): does not make any
 * sense whatsoever anyway)
 */
class ModusDefault {
 public:
  // Never needs a virtual destructor.

  // Missing functions for concrete Modus implementations:
  // \todo(JB): Why is the function below commented out?
  // double initial_conditions(Particles *particles);

  /** Enforces sensible positions for the particles.
   *
   * Currently, this is only needed for BoxModus; the other Modi do
   * nothing.
   *
   * \see BoxModus::impose_boundary_conditions
   */
  int impose_boundary_conditions(Particles* /*p*/,
                                 const OutputsList& /*out_list*/ = {}) {
    return 0;
  }

  /// \return Checks if modus is collider; overwritten in ColliderModus
  bool is_collider() const { return false; }
  /// \return Checks if modus is a box; overwritten in BoxModus
  bool is_box() const { return false; }
  /// \return Checks if modus is list modus; overwritten in ListModus
  bool is_list() const { return false; }
  /// \return Checks if modus is sphere modus; overwritten in SphereModus
  bool is_sphere() const { return false; }
  /// \return Center of mass energy per nucleon pair in ColliderModus
  double sqrt_s_NN() const { return 0.; }
  /// \return The impact parameter; overwritten in ColliderModus
  double impact_parameter() const { return -1.; }
  /// sample impact parameter for collider modus
  void sample_impact() const {}
  /** \return The beam velocity of the projectile required in the Collider
   * modus. In the other modus, return zero. */
  double velocity_projectile() const { return 0.0; }
  /** \return The beam velocity of the target required in the Collider modus.
   * In the other modus, return zero. */
  double velocity_target() const { return 0.0; }
  /** \return The type of Fermi motion required in the Collider modus. In the
   * other modus, just return FermiMotion::Off. */
  FermiMotion fermi_motion() const { return FermiMotion::Off; }
  /// \return Maximal timestep accepted by this modus. Negative means infinity.
  double max_timestep(double) const { return -1.; }
  /// \return equilibration time of the box; overwritten in BoxModus
  double equilibration_time() const { return -1.; }
  /// \return length of the box; overwritten in BoxModus
  double length() const { return -1.; }
  /// \return radius of the sphere; overwritten in SphereModus
  double radius() const { return -1.; }
  /** \return Whether the calculation frame is fixed target;
   *  overwritten in ColliderModus */
  bool calculation_frame_is_fixed_target() const { return false; }
  /**
   * Get the passing time of the two nuclei in a collision. This time
   * corresponds to the moment when the nuclei have just passed entirely
   * through each other and all primary collisions have occured.
   * Formula taken from: Eq. (1) in \iref{Karpenko:2015xea}
   *
   * Only used in ColliderModus for IC output.
   * \return passing_time
   */
  double nuclei_passing_time() const { return 0.0; }

  /**
   * Creates the Grid with normal boundary conditions.
   *
   * \param[in] particles The Particles object containing all particles of the
   *                  currently running Experiment.
   * \param[in] min_cell_length The minimal length of the grid cells.
   * \param[in] timestep_duration Duration of the timestep. It is necessary for
   * formation times treatment: if particle is fully or partially formed before
   * the end of the timestep, it has to be on the grid.
   * \param[in] crit Collision criterion (decides if cell number can be limited)
   * \param[in] include_unformed_particles include unformed particles from
                                           the grid (worsens runtime, necessary
                                           for IC output)
   * \param[in] strategy The strategy to determine the cell size \return the
   * Grid object
   *
   * \see Grid::Grid
   */
  Grid<GridOptions::Normal> create_grid(
      const Particles& particles, double min_cell_length,
      double timestep_duration, CollisionCriterion crit,
      const bool include_unformed_particles,
      CellSizeStrategy strategy = CellSizeStrategy::Optimal) const {
    CellNumberLimitation limit = CellNumberLimitation::ParticleNumber;
    if (crit == CollisionCriterion::Stochastic) {
      limit = CellNumberLimitation::None;
    }
    return {particles,
            min_cell_length,
            timestep_duration,
            limit,
            include_unformed_particles,
            strategy};
  }

  /**
   * Creates GrandCanThermalizer
   *
   * \param[in] conf configuration object
   * \return unique pointer to created thermalizer class
   */
  std::unique_ptr<GrandCanThermalizer> create_grandcan_thermalizer(
      Configuration& conf) const {
    /* Lattice is placed such that the center is 0,0,0.
       If one wants to have a central cell with center at 0,0,0 then
       number of cells should be odd (2k+1) in every direction.
     */

    /*!\Userguide
     * \page input_forced_thermalization_ Forced Thermalization
     * \key Lattice_Sizes (list of 3 doubles, required, no default): \n
     * The lattice is placed such that the center is [0.0,0.0,0.0].
     * If one wants to have a central cell with center at [0.0,0.0,0.0] then
     * number of cells should be odd (2k+1) in every direction. \n
     * Lattice_Sizes is required for all modi, except the box modus. In
     * case of box modus it is set up automatically to match the box size,
     * and the user should not (and is not allowed to) specify it.\n
     * \n
     * Example: Configure Forced Thermalization
     * --------------
     * Forced Thermalization for certain regions is applied if the corresponding
     * section is present in the configuration file. The following example
     * activates forced thermalization in cells in which the energy density is
     * above 0.3 GeV/fm^3. The lattice is initialized with 21 cells in x and y
     * direction and 101 cells in z-direction. The lattice size is 20 fm in x
     and
     * y direction and 50 fm in z-direction. The thermalization is applied only
     * for times later than 10 fm with a timestep of 1 fm/c. The sampling is
     done
     * according to the "biased BF" algorithm.
     * \n
     *\verbatim
     Forced_Thermalization:
         Lattice_Sizes:    [20.0, 20.0, 50.0]
         Cell_Number:    [21, 21, 101]
         Critical_Edens: 0.3
         Start_Time: 10.0
         Timestep: 1.0
         Algorithm: "biased BF"
     \endverbatim
     */

    const std::array<double, 3> l = conf.take({"Lattice_Sizes"});
    const std::array<double, 3> origin = {-0.5 * l[0], -0.5 * l[1],
                                          -0.5 * l[2]};
    const bool periodicity = false;
    return make_unique<GrandCanThermalizer>(conf, l, origin, periodicity);
  }

  /**
   * \ingroup exception
   *  BadInput is an error to throw if the configuration options are invalid.
   */
  struct BadInput : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };
  /**
   * \ingroup exception
   * Thrown when the requested energy is smaller than the masses
   * of two particles.
   */
  struct InvalidEnergy : public BadInput {
    using BadInput::BadInput;
  };
};

}  // namespace smash

#endif  // SRC_INCLUDE_SMASH_MODUSDEFAULT_H_
