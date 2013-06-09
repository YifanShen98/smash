/*
 *    Copyright (c) 2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 */
#ifndef SRC_INCLUDE_PARTICLES_H_
#define SRC_INCLUDE_PARTICLES_H_

#include <cstdio>

#include <map>
#include <vector>

/* necessary forward declarations */
class ParticleData;
class FourVector;
class ParticleType;

/* boost_CM - boost to center of momentum */
void boost_CM(ParticleData *particle1, ParticleData *particle2,
  FourVector *velocity);

/* boost_from_CM - boost back from center of momentum */
void boost_back_CM(ParticleData *particle1, ParticleData *particle2,
  FourVector *velocity_orig);

/* particle_distance - measure distance between two particles */
double particle_distance(ParticleData *particle_orig1,
  ParticleData *particle_orig2);

/* time_collision - measure collision time of two particles */
double collision_time(ParticleData *particle1, ParticleData *particle2);

/* momenta_exchange - soft scattering */
void momenta_exchange(ParticleData *particle1, ParticleData *particle2,
  const float &particle1_mass, const float &particle2_mass);

/* resonance_cross_section - energy-dependent cross section
 * for producing a resonance
 */
double resonance_cross_section(ParticleData *particle1, ParticleData *particle2,
  ParticleType *type_particle1, ParticleType *type_particle2,
  std::vector<ParticleType> *type_list);

/* 1->2 resonance decay process */
size_t resonance_decay(std::vector<ParticleData> *particles,
  std::vector<ParticleType> *types, std::map<int, int> *map_type,
  int *particle_id);

/* 2->1 resonance formation process */
size_t resonance_formation(std::vector<ParticleData> *particles,
  std::vector<ParticleType> *types, std::map<int, int> *map_type,
                         int *particle_id, int *other_id);

#endif  // SRC_INCLUDE_PARTICLES_H_
