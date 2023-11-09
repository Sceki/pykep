/*****************************************************************************
 *   Copyright (C) 2004-2018 The pykep development team,                     *
 *   Advanced Concepts Team (ACT), European Space Agency (ESA)               *
 *                                                                           *
 *   https://gitter.im/esa/pykep                                             *
 *   https://github.com/esa/pykep                                            *
 *                                                                           *
 *   act@esa.int                                                             *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>
#include <iomanip>
#include <iostream>

#include <keplerian_toolbox/astro_constants.hpp>
#include <keplerian_toolbox/core_functions/array3D_operations.hpp>
#include <keplerian_toolbox/core_functions/propagate_lagrangian.hpp>
#include <keplerian_toolbox/lambert_problem.hpp>

using namespace std;
using namespace kep_toolbox;
int main()
{
    // Preamble
    array3D r1, r2;
    double tof;
    boost::mt19937 rng(1234);
    boost::uniform_int<> dist(0, 1);
    boost::variate_generator<boost::mt19937 &, boost::uniform_int<>> rand_bit(rng, dist);
    boost::uniform_real<> dist1(-2, 2);
    boost::variate_generator<boost::mt19937 &, boost::uniform_real<>> drng(rng, dist1);
    double acc = 0, err_max = 0;
    int count = 0;

    // Experiment Settings
    unsigned int Ntrials = 120000;

    // Start Experiment
    for (unsigned int i = 0; i < Ntrials; ++i) {
        // 1 - generate a random problem geometry
        r1[0] = drng();
        r1[1] = drng();
        r1[2] = drng();
        r2[0] = drng();
        r2[1] = drng();
        r2[2] = drng();
        tof = (drng() + 2) / 4 * 100 + 0.1;

        // 2 - Solve the lambert problem
        try {
            double mu = 1.0;
            int revs_max = 20;
            int cw = rand_bit();
            lambert_problem lp(r1, r2, tof, mu, cw, revs_max);

            // 3 - Check its precision using propagate_lagrangian
            for (unsigned int j = 0; j < lp.get_v1().size(); ++j) {
                array3D r1_p(r1), v1_p, err;
                v1_p = lp.get_v1()[j];
                propagate_lagrangian(r1_p, v1_p, tof, mu);
                diff(err, r2, r1_p);
                if (norm(err) > 1e-2) {
                    std::cout << r1 << "\n" << r2 << "\n" << tof;
                }
                err_max = std::max(err_max, norm(err));
                acc += norm(err);
            }
            count += (lp.get_Nmax() * 2 + 1);
        } catch (...) {
            std::cout << "failed: " << std::endl;
            std::cout << "r1[0]=" << r1[0] << "; r1[1]=" << r1[1] << "; r1[2]=" << r1[2] << std::endl;
            std::cout << "r2[0]=" << r2[0] << "; r2[1]=" << r2[1] << "; r2[2]=" << r1[2] << std::endl;
            std::cout << "tof=" << tof << std::endl;
        }
    }
    std::cout << "Max error: " << err_max << std::endl;
    std::cout << "Average Error: " << acc / count << std::endl;
    std::cout << "Number of Problems Solved: " << count << std::endl;
    if (err_max < 1e-6) {
        return 0;
    } else {
        return 1;
    }
}
