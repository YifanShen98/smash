/*
 *
 *    Copyright (c) 2014-2022
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include <vir/test.h>  // This include has to be first

#include "setup.h"

#include <smash/config.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../include/smash/binaryoutput.h"
#include "../include/smash/clock.h"
#include "../include/smash/file.h"
#include "../include/smash/outputinterface.h"
#include "../include/smash/processbranch.h"
#include "../include/smash/scatteraction.h"

using namespace smash;

static const bf::path testoutputpath = bf::absolute(SMASH_TEST_OUTPUT_PATH);

TEST(directory_is_created) {
  bf::create_directories(testoutputpath);
  VERIFY(bf::exists(testoutputpath));
}

TEST(init_particletypes) { Test::create_smashon_particletypes(); }

static const int current_format_version = 7;

/* A set of convenient functions to read binary */

static void read_binary(std::string &s, const FilePtr &file) {
  std::int32_t size = s.size();
  COMPARE(std::fread(&size, sizeof(std::int32_t), 1, file.get()), 1u);
  std::vector<char> buf(size);
  COMPARE(std::fread(&buf[0], 1, size, file.get()), static_cast<size_t>(size));
  s.assign(&buf[0], size);
}

static void read_binary(FourVector &v, const FilePtr &file) {
  COMPARE(std::fread(v.begin(), sizeof(*v.begin()), 4, file.get()), 4u);
}

static void read_binary(std::int32_t &x, const FilePtr &file) {
  COMPARE(std::fread(&x, sizeof(x), 1, file.get()), 1u);
}

static void read_binary(double &x, const FilePtr &file) {
  COMPARE(std::fread(&x, sizeof(x), 1, file.get()), 1u);
}

/* Function to read and compare particle */
static bool compare_particle(const ParticleData &p, const FilePtr &file) {
  int id, pdgcode, charge;
  double mass;
  FourVector pos, mom;
  read_binary(pos, file);
  read_binary(mass, file);
  read_binary(mom, file);
  read_binary(pdgcode, file);
  read_binary(id, file);
  read_binary(charge, file);
  // std::cout << p.id() << " " << id << std::endl;
  // std::cout << p.pdgcode().get_decimal() << " " << pdgcode << std::endl;
  return (p.id() == id) && (p.pdgcode().get_decimal() == pdgcode) &&
         (pos == p.position()) &&
         (mom == p.momentum() && charge == p.type().charge());
}

/* Reads and compares particle in case of extended format */
static void compare_particle_extended(const ParticleData &p,
                                      const FilePtr &file) {
  VERIFY(compare_particle(p, file));
  int collisions_per_particle, id_process, process_type, p1pdg, p2pdg;
  double formation_time, xs_scaling_factor, time_last_collision;
  const auto h = p.get_history();
  read_binary(collisions_per_particle, file);
  read_binary(formation_time, file);
  read_binary(xs_scaling_factor, file);
  read_binary(id_process, file);
  read_binary(process_type, file);
  read_binary(time_last_collision, file);
  read_binary(p1pdg, file);
  read_binary(p2pdg, file);
  COMPARE(collisions_per_particle, h.collisions_per_particle);
  COMPARE(formation_time, p.formation_time());
  COMPARE(xs_scaling_factor, p.xsec_scaling_factor());
  COMPARE(id_process, static_cast<int>(h.id_process));
  COMPARE(process_type, static_cast<int>(h.process_type));
  COMPARE(time_last_collision, h.time_last_collision);
  COMPARE(p1pdg, h.p1.get_decimal());
  COMPARE(p2pdg, h.p2.get_decimal());
}

/* function to read and compare particle block header */
static bool compare_particles_block_header(const int &npart,
                                           const FilePtr &file) {
  int npart_read;
  char c_read;
  COMPARE(std::fread(&c_read, sizeof(char), 1, file.get()), 1u);
  read_binary(npart_read, file);
  // std::cout << c_read << std::endl;
  // std::cout << npart_read << " " << npart << std::endl;
  return (c_read == 'p') && (npart_read == npart);
}

/* function to read and compare collision block header */
static bool compare_interaction_block_header(const int &nin, const int &nout,
                                             const Action &action, double rho,
                                             const FilePtr &file) {
  int nin_read, nout_read, process_type_read;
  double rho_read, weight_read, partial_weight_read;
  char c_read;
  int process_type = static_cast<int>(action.get_type());
  COMPARE(std::fread(&c_read, sizeof(char), 1, file.get()), 1u);
  read_binary(nin_read, file);
  read_binary(nout_read, file);
  COMPARE(std::fread(&rho_read, sizeof(double), 1, file.get()), 1u);
  COMPARE(std::fread(&weight_read, sizeof(double), 1, file.get()), 1u);
  COMPARE(std::fread(&partial_weight_read, sizeof(double), 1, file.get()), 1u);
  read_binary(process_type_read, file);
  // std::cout << c_read << std::endl;
  // std::cout << nin_read << " " << nin << std::endl;
  // std::cout << nout_read << " " << nout << std::endl;
  // std::cout << rho << std::endl;
  return (c_read == 'i') && (nin_read == nin) && (nout_read == nout) &&
         (rho_read == rho) && (weight_read == action.get_total_weight()) &&
         (partial_weight_read == action.get_partial_weight()) &&
         (process_type_read == process_type);
}

/* function to read and compare event end line */
static bool compare_final_block_header(const int ev,
                                       const double impact_parameter,
                                       const bool empty_event,
                                       const FilePtr &file) {
  int ev_read;
  char c_read;
  double b_read;
  char empty_event_read;
  COMPARE(std::fread(&c_read, sizeof(char), 1, file.get()), 1u);
  read_binary(ev_read, file);
  COMPARE(std::fread(&b_read, sizeof(double), 1, file.get()), 1u);
  COMPARE(std::fread(&empty_event_read, sizeof(char), 1, file.get()), 1u);
  return (c_read == 'f') && (ev_read == ev) && (b_read == impact_parameter) &&
         (empty_event_read == empty_event);
}

/* function to check we reached the end of the file */
static bool check_end_of_file(const FilePtr &file) {
  return std::feof(file.get()) == 0;
}

TEST(fullhistory_format) {
  /* create two smashon particles */
  Particles particles;
  const ParticleData p1 = particles.insert(Test::smashon_random());
  const ParticleData p2 = particles.insert(Test::smashon_random());

  /* Create elastic interaction (smashon + smashon). */
  const int event_id = 0;
  const double impact_parameter = 1.473;
  const bool empty_event = false;
  EventInfo event = Test::default_event_info(impact_parameter, empty_event);
  ScatterActionPtr action = make_unique<ScatterAction>(p1, p2, 0.);
  action->add_all_scatterings(10., true, Test::all_reactions_included(),
                              Test::no_multiparticle_reactions(), 0., true,
                              false, false, NNbarTreatment::NoAnnihilation, 1.0,
                              0.0);
  action->generate_final_state();
  ParticleList final_particles = action->outgoing_particles();
  const double rho = 0.123;

  const bf::path collisionsoutputfilepath =
      testoutputpath / "collisions_binary.bin";
  bf::path collisionsoutputfilepath_unfinished = collisionsoutputfilepath;
  collisionsoutputfilepath_unfinished += ".unfinished";
  {
    /* Set the most verbose option */
    OutputParameters output_par = OutputParameters();
    output_par.coll_printstartend = true;
    output_par.coll_extended = false;

    /* Create an instance of binary output */
    auto bin_output = make_unique<BinaryOutputCollisions>(
        testoutputpath, "Collisions", output_par);
    VERIFY(bf::exists(collisionsoutputfilepath_unfinished));

    /* Write initial state output: the two smashons we created */
    bin_output->at_eventstart(particles, event_id, event);
    bin_output->at_interaction(*action, rho);

    /* Final state output */
    action->perform(&particles, 1);
    bin_output->at_eventend(particles, event_id, event);
  }
  VERIFY(!bf::exists(collisionsoutputfilepath_unfinished));
  VERIFY(bf::exists(collisionsoutputfilepath));

  /*
   * Now we have an artificially generated binary output.
   * Let us try if we can read and understand it.
   */

  {
    FilePtr binF = fopen(collisionsoutputfilepath.native(), "rb");
    VERIFY(binF.get());
    // Header
    std::vector<char> buf(4);
    std::string magic, smash_version;
    int format_version_number;

    COMPARE(std::fread(&buf[0], 1, 4, binF.get()), 4u);  // magic number
    magic.assign(&buf[0], 4);
    read_binary(format_version_number, binF);  // format version number
    read_binary(smash_version, binF);          // smash version

    COMPARE(magic, "SMSH");
    COMPARE(format_version_number, current_format_version);
    COMPARE(smash_version, SMASH_VERSION);

    // particles at event start: expect two smashons
    VERIFY(compare_particles_block_header(2, binF));
    VERIFY(compare_particle(p1, binF));
    VERIFY(compare_particle(p2, binF));

    // interaction: 2 smashons -> 2 smashons
    VERIFY(compare_interaction_block_header(2, 2, *action, rho, binF));
    VERIFY(compare_particle(p1, binF));
    VERIFY(compare_particle(p2, binF));
    VERIFY(compare_particle(final_particles[0], binF));
    VERIFY(compare_particle(final_particles[1], binF));

    // paricles at event end: two smashons
    VERIFY(compare_particles_block_header(2, binF));
    VERIFY(compare_particle(final_particles[0], binF));
    VERIFY(compare_particle(final_particles[1], binF));

    // event end line
    VERIFY(compare_final_block_header(event_id, impact_parameter, empty_event,
                                      binF));
    VERIFY(check_end_of_file(binF));
  }

  VERIFY(bf::remove(collisionsoutputfilepath));
}

TEST(particles_format) {
  /* create two smashon particles */
  const auto particles =
      Test::create_particles(2, [] { return Test::smashon_random(); });
  const int event_id = 0;
  const double impact_parameter = 4.382;
  const bool empty_event = false;
  EventInfo event = Test::default_event_info(impact_parameter, empty_event);
  const ParticleList initial_particles = particles->copy_to_vector();

  const bf::path particleoutputpath = testoutputpath / "particles_binary.bin";
  bf::path particleoutputpath_unfinished = particleoutputpath;
  particleoutputpath_unfinished += ".unfinished";
  {
    /* Set the most verbose option */
    OutputParameters output_par = OutputParameters();
    output_par.part_extended = false;
    output_par.part_only_final = OutputOnlyFinal::No;
    /* Create an instance of binary output */
    auto bin_output = make_unique<BinaryOutputParticles>(
        testoutputpath, "Particles", output_par);
    VERIFY(bool(bin_output));
    VERIFY(bf::exists(particleoutputpath_unfinished));

    /* Write initial state output: the two smashons we created */
    bin_output->at_eventstart(*particles, event_id, event);
    /* Interaction smashon + smashon -> smashon */
    ParticleList final_state = {Test::smashon_random()};
    particles->replace(initial_particles, final_state);

    DensityParameters dens_par(Test::default_parameters());
    bin_output->at_intermediate_time(*particles, nullptr, dens_par, event);

    /* Final state output */
    bin_output->at_eventend(*particles, event_id, event);
  }
  const ParticleList final_particles = particles->copy_to_vector();
  VERIFY(!bf::exists(particleoutputpath_unfinished));
  VERIFY(bf::exists(particleoutputpath));

  /*
   * Now we have an artificially generated binary output.
   * Let us try if we can read and understand it.
   */

  {
    FilePtr binF = fopen(particleoutputpath.native(), "rb");
    VERIFY(binF.get());
    // Header
    std::vector<char> buf(4);
    std::string magic, smash_version;
    int format_version_number;

    COMPARE(std::fread(&buf[0], 1, 4, binF.get()), 4u);  // magic number
    magic.assign(&buf[0], 4);
    read_binary(format_version_number, binF);  // format version number
    read_binary(smash_version, binF);          // smash version

    COMPARE(magic, "SMSH");
    COMPARE(format_version_number, current_format_version);
    COMPARE(smash_version, SMASH_VERSION);

    int npart;
    // particles at event start: expect two smashons
    npart = 2;
    VERIFY(compare_particles_block_header(npart, binF));
    VERIFY(compare_particle(initial_particles[0], binF));
    VERIFY(compare_particle(initial_particles[1], binF));

    // Periodic output: already after interaction. One smashon expected.
    npart = 1;
    VERIFY(compare_particles_block_header(npart, binF));
    VERIFY(compare_particle(final_particles[0], binF));

    // particles at event end
    npart = 1;
    VERIFY(compare_particles_block_header(npart, binF));
    VERIFY(compare_particle(final_particles[0], binF));

    // after end of event
    VERIFY(compare_final_block_header(event_id, impact_parameter, empty_event,
                                      binF));
    VERIFY(check_end_of_file(binF));
  }

  VERIFY(bf::remove(particleoutputpath));
}

TEST(extended) {
  /* create two smashon particles */
  Particles particles;
  const ParticleData p1 = particles.insert(Test::smashon_random());
  const ParticleData p2 = particles.insert(Test::smashon_random());

  /* Create elastic interaction (smashon + smashon). */
  ScatterActionPtr action = make_unique<ScatterAction>(p1, p2, 0.);
  action->add_all_scatterings(10., true, Test::all_reactions_included(),
                              Test::no_multiparticle_reactions(), 0., true,
                              false, false, NNbarTreatment::NoAnnihilation, 1.0,
                              0.0);
  action->generate_final_state();
  ParticleList final_particles = action->outgoing_particles();
  const double rho = 0.123;

  const int event_id = 0;
  const double impact_parameter = 1.473;
  const bool empty_event = true;
  EventInfo event = Test::default_event_info(impact_parameter, empty_event);

  const bf::path collisionsoutputfilepath =
      testoutputpath / "collisions_binary.bin";
  bf::path collisionsoutputfilepath_unfinished = collisionsoutputfilepath;
  collisionsoutputfilepath_unfinished += ".unfinished";
  {
    OutputParameters output_par = OutputParameters();
    output_par.coll_printstartend = true;
    output_par.coll_extended = true;

    /* Create an instance of binary output */
    auto bin_output = make_unique<BinaryOutputCollisions>(
        testoutputpath, "Collisions", output_par);
    VERIFY(bf::exists(collisionsoutputfilepath_unfinished));

    /* Write initial state output: the two smashons we created */
    bin_output->at_eventstart(particles, event_id, event);
    bin_output->at_interaction(*action, rho);

    /* Final state output */
    action->perform(&particles, 1);
    bin_output->at_eventend(particles, event_id, event);
  }
  VERIFY(!bf::exists(collisionsoutputfilepath_unfinished));
  VERIFY(bf::exists(collisionsoutputfilepath));

  /*
   * Now we have an artificially generated binary output.
   * Let us try if we can read and understand it.
   */

  {
    FilePtr binF = fopen(collisionsoutputfilepath.native(), "rb");
    VERIFY(binF.get());
    // Header
    std::vector<char> buf(4);
    std::string magic, smash_version;
    uint16_t format_version_number, extended_version;

    COMPARE(std::fread(&buf[0], 1, 4, binF.get()), 4u);  // magic number
    magic.assign(&buf[0], 4);
    VERIFY(std::fread(&format_version_number, sizeof(format_version_number), 1,
                      binF.get()) == 1);
    VERIFY(std::fread(&extended_version, sizeof(extended_version), 1,
                      binF.get()) == 1);
    read_binary(smash_version, binF);  // smash version

    COMPARE(magic, "SMSH");
    COMPARE(static_cast<int>(format_version_number), current_format_version);
    COMPARE(extended_version, 1);
    COMPARE(smash_version, SMASH_VERSION);

    // particles at event atart: expect two smashons
    VERIFY(compare_particles_block_header(2, binF));
    compare_particle_extended(p1, binF);
    compare_particle_extended(p2, binF);

    // interaction: 2 smashons -> 2 smashons
    VERIFY(compare_interaction_block_header(2, 2, *action, rho, binF));
    compare_particle_extended(p1, binF);
    compare_particle_extended(p2, binF);
    compare_particle_extended(final_particles[0], binF);
    compare_particle_extended(final_particles[1], binF);

    // paricles at event end: two smashons
    VERIFY(compare_particles_block_header(2, binF));
    for (const auto &particle : particles) {
      compare_particle_extended(particle, binF);
    }

    // event end line
    VERIFY(compare_final_block_header(event_id, impact_parameter, empty_event,
                                      binF));
    VERIFY(check_end_of_file(binF));
  }

  VERIFY(bf::remove(collisionsoutputfilepath));
}

TEST(initial_conditions_format) {
  // Create 1 particle
  Particles particles;
  ParticleData p1 = particles.insert(Test::smashon_random());
  p1.set_4position(FourVector(2.3, 1.35722, 1.42223, 1.5));  // tau = 1.74356

  // Create and perform action ("hypersurface crossing")
  ActionPtr action = make_unique<HypersurfacecrossingAction>(p1, p1, 0.0);
  action->generate_final_state();
  action->perform(&particles, 1);

  const int event_id = 0;
  const bool empty_event = false;
  const double impact_parameter = 0.0;
  EventInfo event = Test::default_event_info(impact_parameter, empty_event);

  const bf::path particleoutputpath = testoutputpath / "SMASH_IC.bin";
  bf::path particleoutputpath_unfinished = particleoutputpath;
  particleoutputpath_unfinished += ".unfinished";

  {
    OutputParameters output_par = OutputParameters();
    output_par.part_extended = false;
    double density = 0.0;
    /* Create an instance of binary output */
    auto bin_output = make_unique<BinaryOutputInitialConditions>(
        testoutputpath, "SMASH_IC", output_par);
    VERIFY(bool(bin_output));
    VERIFY(bf::exists(particleoutputpath_unfinished));

    /* Write event start information: This should do nothing for IC output */
    bin_output->at_interaction(*action, density);

    /* Write particle line for hypersurface crossing */
    bin_output->at_interaction(*action, density);

    /* Event end output */
    bin_output->at_eventend(particles, event_id, event);
  }
  VERIFY(!bf::exists(particleoutputpath_unfinished));
  VERIFY(bf::exists(particleoutputpath));

  /* Read the afore created output */
  {
    FilePtr binF = fopen(particleoutputpath.native(), "rb");
    VERIFY(binF.get());
    // Header
    std::vector<char> buf(4);
    std::string magic, smash_version;
    int format_version_number;

    COMPARE(std::fread(&buf[0], 1, 4, binF.get()), 4u);  // magic number
    magic.assign(&buf[0], 4);
    read_binary(format_version_number, binF);  // format version number
    read_binary(smash_version, binF);          // smash version

    COMPARE(magic, "SMSH");
    COMPARE(format_version_number, current_format_version);
    COMPARE(smash_version, SMASH_VERSION);

    int npart = 1;  // expect one particle in output

    VERIFY(compare_particles_block_header(npart, binF));
    VERIFY(compare_particle(p1, binF));

    VERIFY(check_end_of_file(binF));
  }
  VERIFY(bf::remove(particleoutputpath));
}
