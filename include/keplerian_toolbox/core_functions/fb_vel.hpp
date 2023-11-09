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
#ifndef KEP_TOOLBOX_FB_VEL_H
#define KEP_TOOLBOX_FB_VEL_H

#include <cmath>

#include <keplerian_toolbox/planet/base.hpp>

/// Compute fly-by constraints
/**
 * This template function can be used to evaluate the feasibility of a fly-by
 * described by relative planetary velocities
 * before and after.
 *
 * \param[out] dV is the delta-V thrust magnitude needed to make a fly-by
 * possible. For a ballistic fly-by dV must be zero.
 * \param[in] v_rel_in  initial position vector. On output contains the
 * propagated position. (r0[1],r0[2],r0[3] need to be preallocated, suggested
 * template type is std::array<double,3))
 * \param[in] v_rel_out initial velocity vector. On output contains the
 * propagated velocity. (v0[1],v0[2],v0[3] need to be preallocated, suggested
 * template type is std::array<double,3))
 * \param[in] pl a planet object
 *
 * @author Dario Izzo (dario.izzo _AT_ googlemail.com)
 * @author Johannes Simon (johannessimon81_AT_gmail.com)
 */

namespace kep_toolbox
{

template <class vettore3D>
inline void fb_vel(double &dV, const vettore3D &v_rel_in, const vettore3D &v_rel_out, const planet::base &pl)
{
    double Vin2 = v_rel_in[0] * v_rel_in[0] + v_rel_in[1] * v_rel_in[1] + v_rel_in[2] * v_rel_in[2];
    double Vout2 = v_rel_out[0] * v_rel_out[0] + v_rel_out[1] * v_rel_out[1] + v_rel_out[2] * v_rel_out[2];
    // eq_V2 = Vin2 - Vout2;

    double e_min = 1 + pl.get_safe_radius() / pl.get_mu_self() * Vin2;
    double alpha = acos((v_rel_in[0] * v_rel_out[0] + v_rel_in[1] * v_rel_out[1] + v_rel_in[2] * v_rel_out[2])
                        / sqrt(Vin2 * Vout2));
    double ineq_delta = alpha - 2 * asin(1 / e_min);

    if (ineq_delta > 0.0)
        dV = sqrt(Vout2 + Vin2 - 2.0 * sqrt(Vout2 * Vin2) * cos(ineq_delta));
    else
        dV = fabs(sqrt(Vout2) - sqrt(Vin2));

    return;
}
} // namespace end

#endif // KEP_TOOLBOX_FB_VEL_H
