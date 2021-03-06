/******************************************************************************\

                    Copyright 2013-2015. Stanford University.
                              All Rights Reserved.

                                Original Author
                         Yutong Zhao <proteneer@gmail.com>

\******************************************************************************/

#include <OpenMM.h>
#include <stdexcept>
#include <cmath>
#include <boost/format.hpp>

#include "StateTests.h"

using namespace OpenMM;
using std::vector;


void StateTests::checkForNans(const State & state) {
    const vector<Vec3> & positions = state.getPositions();
    const vector<Vec3> & velocities = state.getVelocities();
    const vector<Vec3> & forces = state.getForces();

    for (unsigned i = 0; i < positions.size(); i++)
        for (int j = 0; j < 3; j++)
            if (std::isnan(positions[i][j]))
                throw std::runtime_error(boost::str(boost::format("NaNs detected in positions: (%1%, %2%)") % i % j));

    for (unsigned i = 0; i < velocities.size(); i++)
        for (int j = 0; j < 3; j++)
            if (std::isnan(velocities[i][j]))
                throw std::runtime_error(boost::str(boost::format("NaNs detected in velocities: (%1%, %2%)") % i % j));

    for (unsigned i = 0; i < forces.size(); i++)
        for (int j = 0; j < 3; j++)
            if (std::isnan(forces[i][j]))
                throw std::runtime_error(boost::str(boost::format("NaNs detected in forces: (%1%, %2%)") % i % j));
}


void StateTests::checkForDiscrepancies(const State & state) {
    const vector<Vec3> & velocities = state.getVelocities();
    const vector<Vec3> & forces = state.getForces();
    Vec3 a, b, c;

    state.getPeriodicBoxVectors(a, b, c);

    // velocities in a Boltzmann-Maxwell distribution has standard deviation
    // equal to roughly sqrt(0.00831451121 * temperature / mass)
    // so if temperature is around 370 Kelvin, mass is 1.008 (for H, the lightest
    // atom) we have a Gaussian distribution with a standard deviation of 1.747
    // a 10 sigma events, ie. 10*1.747=17.47, which should occur once in 390.68
    // billion times

    unsigned zeroVelocityCount = 0;
    for (unsigned i = 0; i < velocities.size(); i++) {
        for (int j = 0; j < 3; j++)
            if (fabs(velocities[i][j]) > 17.47)
                throw std::runtime_error(boost::str(boost::format("Discrepancy: Velocities are blowing up: (%1%, %2%) = %3%") % i % j % velocities[i][j]));

        for (int j = 0; j < 3; j++)
            if (velocities[i][j] == 0)
                zeroVelocityCount++;
    }

    if (zeroVelocityCount > (3 * velocities.size() / 2))
        throw std::runtime_error("Discrepancy: More than half of all velocities set exactly to zero!");

    for (unsigned i = 0; i < forces.size(); i++)
        for (int j = 0; j < 3; j++)
            if (fabs(forces[i][j]) > 50000)
                throw std::runtime_error(boost::str(boost::format("Discrepancy: Forces are blowing up: (%1%, %2%)") % i % j));
}


void StateTests::compareEnergies(const State & first, const State & b,
                                 double tolerance) {
    double potentialEnergyA = first.getPotentialEnergy();
    double potentialEnergyB = b.getPotentialEnergy();
    double diff = fabs(potentialEnergyA - potentialEnergyB);

    if (diff > tolerance)
        throw std::runtime_error(boost::str(boost::format("Potential energy error of %1%, threshold of %2%\nReference potential energy: %3% | Given potential energy: %4%")
                                            % diff % tolerance % potentialEnergyA % potentialEnergyB));

    double kineticEnergyA = first.getKineticEnergy();
    double kineticEnergyB = b.getKineticEnergy();
    diff = fabs(kineticEnergyA - kineticEnergyB);

    if (diff > tolerance)
        throw std::runtime_error(boost::str(boost::format("Kinetic energy error of %1%, threshold of %2%\nReference potential energy: %3% | Given potential energy: %4%")
                                            % diff % tolerance % kineticEnergyA % kineticEnergyB));
}


void StateTests::compareForces(const State & first, const State & b,
                               double tolerance) {
    const vector<Vec3> & forcesA = first.getForces();
    const vector<Vec3> & forcesB = b.getForces();
    int nAtoms = forcesA.size();
    double mse = 0;

    for (int i = 0; i < nAtoms; i++) {
        double magnitudeA =
            sqrt(forcesA[i][0] * forcesA[i][0] + forcesA[i][1] * forcesA[i][1] +
                 forcesA[i][2] * forcesA[i][2]);
        double magnitudeB =
            sqrt(forcesB[i][0] * forcesB[i][0] + forcesB[i][1] * forcesB[i][1] +
                 forcesB[i][2] * forcesB[i][2]);
        double error = magnitudeA - magnitudeB;
        mse += error * error;
    }

    mse = sqrt(mse / nAtoms);

    if (mse > tolerance)
        throw std::runtime_error(boost::str(boost::format("Force RMSE error of %1% with threshold of %2%") % mse % tolerance));
}


void StateTests::compareForcesAndEnergies(const State & a, const State & b,
        double forceTolerance,
        double energyTolerance) {
    compareForces(a, b);
    compareEnergies(a, b);
}
