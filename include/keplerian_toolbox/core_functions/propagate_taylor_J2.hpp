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

#ifndef KEP_TOOLBOX_PROPAGATE_TAYLOR_J2_H
#define KEP_TOOLBOX_PROPAGATE_TAYLOR_J2_H

#include <algorithm>
#include <boost/array.hpp>
#include <cmath>

#include <keplerian_toolbox/astro_constants.hpp>
#include <keplerian_toolbox/exceptions.hpp>

namespace kep_toolbox
{

template <class T>
double propagate_taylor_J2_step(T &r0, T &v0, double &m0, const double &h, const int &order, const T &thrust,
                                const double &mu, const double &veff, const double &J2RG2, const double &xm,
                                const double &eps_a, const double &eps_r, std::vector<std::array<double, 7>> &x,
                                std::vector<std::array<double, 34>> &u)
{

    // We initialize the initial conditions
    x[0][0] = r0[0];
    x[0][1] = r0[1];
    x[0][2] = r0[2];
    x[0][3] = v0[0];
    x[0][4] = v0[1];
    x[0][5] = v0[2];
    x[0][6] = m0;

    // We compute all needed taylor coefficients via automated differentiation
    int n = 0;
    double alpha = -1.5; // Exponent for r^2
    double beta = -1.;   // Exponent for m and r^2
    double sqrtT = sqrt(thrust[0] * thrust[0] + thrust[1] * thrust[1] + thrust[2] * thrust[2]);
    while (n < order) {
        u[n][0] = x[n][0]; // x
        u[n][1] = x[n][1]; // y
        u[n][2] = x[n][2]; // z
        u[n][3] = x[n][3]; // vx
        u[n][4] = x[n][4]; // vy
        u[n][5] = x[n][5]; // vz
        u[n][6] = x[n][6]; // m
        for (int j = 0; j <= n; j++)
            u[n][7] += u[j][0] * u[n - j][0]; // x^2
        for (int j = 0; j <= n; j++)
            u[n][8] += u[j][1] * u[n - j][1]; // y^2
        for (int j = 0; j <= n; j++)
            u[n][9] += u[j][2] * u[n - j][2]; // z^2
        u[n][10] = u[n][7] + u[n][8];         // x^2+y^2
        u[n][11] = u[n][10] + u[n][9];        // r^2

        if (n == 0) {
            u[n][12] = sqrt(1. / (u[n][11] * u[n][11] * u[n][11]));
        } else {
            for (int j = 0; j < n; ++j)
                u[n][12] += (alpha * n - j * (alpha + 1)) * u[n - j][11] * u[j][12];
            u[n][12] /= n * u[0][11];
        } // 1 / r^3

        u[n][13] = -u[n][12] * mu; //-mu/r^3

        for (int j = 0; j <= n; j++)
            u[n][14] += u[j][0] * u[n - j][13]; //-mu x /r^3
        for (int j = 0; j <= n; j++)
            u[n][15] += u[j][1] * u[n - j][13]; //-mu y /r^3
        for (int j = 0; j <= n; j++)
            u[n][16] += u[j][2] * u[n - j][13]; //-mu z /r^3

        if (n == 0) {
            u[n][17] = 1. / u[0][6];
        } else {
            for (int j = 0; j < n; ++j) {
                u[n][17] += (beta * n - j * (beta + 1.)) * u[n - j][6] * u[j][17];
            }
            u[n][17] /= n * u[0][6];
        } // 1/m

        // We compute the terms needed for the J2 case
        (n == 0) ? u[n][18] = 1. : u[n][18] = 0.; // the constant 1.

        if (n == 0) {
            u[n][19] = 1. / u[0][11];
        } else {
            for (int j = 0; j < n; ++j) {
                u[n][19] += (beta * n - j * (beta + 1.)) * u[n - j][11] * u[j][19];
            }
            u[n][19] /= n * u[0][11];
        }                                      // 1/r^2
        u[n][20] = 3. / 2. * J2RG2 * u[n][19]; // 3/2 J2 RG^2 / r^2
        for (int j = 0; j <= n; j++)
            u[n][21] += u[j][9] * u[n - j][19]; // z^2 / r^2
        u[n][22] = u[n][18] - 5 * u[n][21];     // 1 - 5 z^2/r^2
        for (int j = 0; j <= n; j++)
            u[n][23] += u[j][20] * u[n - j][22]; // 3/2 J2 RG^2 / r^2 * (1 - 5 z^2/r^2)
        u[n][24] = 3 * u[n][18] - 5 * u[n][21];  // 3 - 5 z^2/r^2
        for (int j = 0; j <= n; j++)
            u[n][25] += u[j][20] * u[n - j][24]; // 3/2 J2 RG^2 / r^2 * (3 - 5 z^2/r^2)
        u[n][26] = u[n][18] + u[n][23];          // 1 + 3/2 J2 RG^2 / r^2 * (1 - 5 z^2/r^2)
        u[n][27] = u[n][18] + u[n][25];          // 1 + 3/2 J2 RG^2 / r^2 * (3 - 5 z^2/r^2)
        for (int j = 0; j <= n; j++)
            u[n][28] += u[j][14] * u[n - j][26]; // -mu/r^3 * x * (1 + 3/2 J2 RG^2 / r^2 * (1 - 5 z^2/r^2))
        for (int j = 0; j <= n; j++)
            u[n][29] += u[j][15] * u[n - j][26]; // -mu/r^3 * y * (1 + 3/2 J2 RG^2 / r^2 * (1 - 5 z^2/r^2))
        for (int j = 0; j <= n; j++)
            u[n][30] += u[j][16] * u[n - j][27]; // -mu/r^3 * z * (1 + 3/2 J2 RG^2 / r^2 * (3 - 5 z^2/r^2))

        // The final equations are:
        u[n][31] = u[n][28] + u[n][17] * thrust[0]; // eq1
        u[n][32] = u[n][29] + u[n][17] * thrust[1]; // eq2
        u[n][33] = u[n][30] + u[n][17] * thrust[2]; // eq3

        // And we may then update the variables
        x[n + 1][0] = 1. / (n + 1) * u[n][3];
        x[n + 1][1] = 1. / (n + 1) * u[n][4];
        x[n + 1][2] = 1. / (n + 1) * u[n][5];
        x[n + 1][3] = 1. / (n + 1) * u[n][31];
        x[n + 1][4] = 1. / (n + 1) * u[n][32];
        x[n + 1][5] = 1. / (n + 1) * u[n][33];
        (n == 0) ? x[n + 1][6] = -sqrtT / veff : x[n + 1][6] = 0;
        n++;
    }

    // We now compute the Taylor expansion, first the optimal step size and then
    // the sum....

    // Determining the optimal step size (see Jorba's method)
    double step, rho_m;

    // The infinity norm of the highest order derivative
    double xm_n = std::max(std::abs(x[n][0]), std::abs(x[n][1]));
    xm_n = std::max(xm_n, std::abs(x[n][2]));
    xm_n = std::max(xm_n, std::abs(x[n][3]));
    xm_n = std::max(xm_n, std::abs(x[n][4]));
    xm_n = std::max(xm_n, std::abs(x[n][5]));
    xm_n = std::max(xm_n, std::abs(x[n][6]));

    // The infinity norm of the n-1 order derivative
    double xm_n1 = std::max(std::abs(x[n - 1][0]), std::abs(x[n - 1][1]));
    xm_n1 = std::max(xm_n1, std::abs(x[n - 1][2]));
    xm_n1 = std::max(xm_n1, std::abs(x[n - 1][3]));
    xm_n1 = std::max(xm_n1, std::abs(x[n - 1][4]));
    xm_n1 = std::max(xm_n1, std::abs(x[n - 1][5]));
    xm_n1 = std::max(xm_n1, std::abs(x[n - 1][6]));

    if (eps_r * xm < eps_a) {
        rho_m = std::min(pow((1 / xm_n), 1. / n), pow((1 / xm_n1), 1. / (n - 1)));
    } else {
        rho_m = std::min(pow((xm / xm_n), 1. / n), pow((xm / xm_n1), 1. / (n - 1)));
    }
    step = rho_m / (M_E * M_E);

    if (h < 0) step = -step;
    if (std::abs(step) > std::abs(h)) step = h;

    // Horner's-Biscani method
    double steppow = step;
    for (int j = 1; j <= order; ++j) {
        r0[0] += x[j][0] * steppow;
        r0[1] += x[j][1] * steppow;
        r0[2] += x[j][2] * steppow;
        v0[0] += x[j][3] * steppow;
        v0[1] += x[j][4] * steppow;
        v0[2] += x[j][5] * steppow;
        steppow *= step;
    }
    m0 += x[1][6] * step;
    return step;
}

/// Taylor series propagation of a constant thrust trajectory
/**
 * This template function propagates an initial state for a time t assuming a
 * central body and a keplerian
 * motion perturbed by an inertially constant thrust u
 *
 * \param[in,out] r0 initial position vector. On output contains the propagated
 * position. (r0[1],r0[2],r0[3] need to be preallocated, suggested template type
 * is std::array<double,3))
 * \param[in,out] v0 initial velocity vector. On output contains the propagated
 * velocity. (v0[1],v0[2],v0[3] need to be preallocated, suggested template type
 * is std::array<double,3))
 * \param[in] T thrust vector (cartesian components)
 * \param[in,out] t propagation time (can be negative). If the maximum number of
 * iterations is reached, the time is returned where the state is calculated for
 * the last time
 * \param[in] mu central body gravitational parameter
 * \param[in] J2RG2 The product between the \f$J_2\f$ coefficient and the radius
 * of the geoid squared
 * \param[in] log10tolerance logarithm of the desired absolute tolerance
 * \param[in] log10rtolerance logarithm of the desired relative tolerance
 * \param[in] max_iter maximum number of iteration allowed
 * \param[in] max_order maximum order for the polynomial expansion
 *
 * \throw value_error if max_iter is hit.....
 * \throw value_error if max_order is exceeded.....
 *
 * NOTE: Equations of motions are written and propagated in cartesian
 * coordinates
 *
 * @author Dario Izzo (dario.izzo _AT_ googlemail.com)
 */
template <class T>
void propagate_taylor_J2(T &r0, T &v0, double &m0, const T &u, const double &t0, const double &mu = 1,
                         const double &veff = 1, const double &J2RG2 = 0., const int &log10tolerance = -10,
                         const int &log10rtolerance = -10, const int &max_iter = 100000, const int &max_order = 3000)
{

    std::array<double, 7> dumb;
    std::array<double, 34> dumb2;
    for (decltype(dumb.size()) i = 0u; i < dumb.size(); ++i)
        dumb[i] = 0;
    for (decltype(dumb.size()) i = 0u; i < dumb2.size(); ++i)
        dumb2[i] = 0;

    std::vector<std::array<double, 7>> _x;  // x[order][var]
    std::vector<std::array<double, 34>> _u; // u[order][var]

    double step = t0;
    double eps_a = pow(10., log10tolerance);
    double eps_r = pow(10., log10rtolerance);
    double eps_m, xm;
    int j;
    for (j = 0; j < max_iter; ++j) {
        // We follow the method described by Jorba in "A software package ...."
        // 1 - We determine eps_m from Eq. (7)
        xm = std::max(std::abs(r0[0]), std::abs(r0[1]));
        xm = std::max(xm, std::abs(r0[2]));
        xm = std::max(xm, std::abs(v0[0]));
        xm = std::max(xm, std::abs(v0[1]));
        xm = std::max(xm, std::abs(v0[2]));
        xm = std::max(xm, std::abs(m0));

        // 2 - We evaluate the polynomial order
        (eps_r * xm < eps_a) ? eps_m = eps_a : eps_m = eps_r;
        int order = (int)(ceil(-0.5 * log(eps_m) + 1));
        if (order > max_order) throw_value_error("Polynomial order is too high.....");

        // 3 - We allocate or deallocate memory if necessary
        _x.assign(order + 1, dumb);
        _u.assign(order, dumb2);
        double h = propagate_taylor_J2_step(r0, v0, m0, step, order, u, mu, veff, J2RG2, xm, eps_a, eps_r, _x, _u);
        if (std::abs(h) >= std::abs(step))
            break;
        else {
            step = step - h;
        }
    }
    if (j > max_iter - 1) throw_value_error("Maximum number of iteration reached");
}

} // Namespace

#endif // KEP_TOOLBOX_PROPAGATE_TAYLOR_J2_H
