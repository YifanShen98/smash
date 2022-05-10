/*
 *
 *    Copyright (c) 2019-
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SMASH_SETUP_PARTICLES_DECAYMODES_H_
#define SRC_INCLUDE_SMASH_SETUP_PARTICLES_DECAYMODES_H_

#include <string>
#include <utility>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace smash {
/**
 * Loads particles and decaymodes from provided files
 * particles_file and decaymodes_file. In case if particles_file
 * or decaymodes_file are nullptr, the defaults are taken
 * \param[in] particles_file a file containing particles list.
 *            See \ref inputparticles.
 * \param[in] decaymodes_file a file containing decay modes of
 *            the resonances. See \ref inputdecaymodes.
 * \return a pair of strings -- the contents of particle
 *             and decaymode files.
 */
std::pair<std::string, std::string> load_particles_and_decaymodes(
    const boost::filesystem::path &particles_file,
    const boost::filesystem::path &decaymodes_file);
/// Loads default smash particle list and decaymodes
void initialize_default_particles_and_decaymodes();

}  // namespace smash

#endif  // SRC_INCLUDE_SMASH_SETUP_PARTICLES_DECAYMODES_H_
