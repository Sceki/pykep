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

#ifndef KEP_TOOLBOX_LEG_S_H
#define KEP_TOOLBOX_LEG_S_H

#include <boost/type_traits/is_same.hpp>
#include <boost/utility.hpp>
#include <iterator>
#include <vector>

#include <keplerian_toolbox/core_functions/array3D_operations.hpp>
#include <keplerian_toolbox/core_functions/propagate_taylor_s.hpp>
#include <keplerian_toolbox/epoch.hpp>
#include <keplerian_toolbox/exceptions.hpp>
#include <keplerian_toolbox/sims_flanagan/sc_state.hpp>
#include <keplerian_toolbox/sims_flanagan/spacecraft.hpp>
#include <keplerian_toolbox/sims_flanagan/throttle.hpp>

#include <keplerian_toolbox/detail/visibility.hpp>
#include <keplerian_toolbox/serialization.hpp>

namespace kep_toolbox
{
namespace sims_flanagan
{

/// Single low-thrust leg (phase) using Sundmann variable
/**
 * This class represents, generically, a low-thrust leg (phase) as a sequence of
 * successive
 * constant low-thrust segments. The segment duration is equally distributed in
 * the pseudo
 * time space \f$dt = c r^\alpha ds \f$
 * The leg achieves to transfer a given spacecraft from an initial to a final
 * state in the
 * pseudo-time given (and can thus be considered as feasible) whenever the method
 * evaluate_mismatch
 * returns all zeros (8 values) and the method get_throttles_con returns all
 * values less than zero.
 * Th sequence of different thrusts is represented by the class throttles. These
 * represent
 * the cartesian components \f$ \mathbf u = (u_x,u_y,u_y) \f$ of a normalized
 * thrust and are thus
 * numbers that need to satisfy the constraint \f$|\mathbf u| \le 1\f$
 *
 * \image html s_leg.png "Visualization of a feasible leg (Earth-Mars)"
 * \image latex s_leg.png "Visualization of a feasible leg (Earth-Mars)"
 * width=5cm
 *
 * @author Dario Izzo (dario.izzo _AT_ googlemail.com)
 */
class KEP_TOOLBOX_DLL_PUBLIC leg_s;
KEP_TOOLBOX_DLL_PUBLIC std::ostream &operator<<(std::ostream &s, const leg_s &in);

class KEP_TOOLBOX_DLL_PUBLIC leg_s
{
    friend KEP_TOOLBOX_DLL_PUBLIC std::ostream &operator<<(std::ostream &s, const leg_s &in);

public:
    std::string human_readable() const;
    /// Constructor.
    /**
     * Default constructor. Constructs a meaningless leg.
     */
    leg_s()
        : m_ti(), m_xi(), m_throttles(), m_tf(), m_xf(), m_sf(0), m_sc(), m_mu(0), m_c(), m_alpha(), m_tol(-10),
          m_states(), m_ceq(), m_cineq(), m_dv()
    {
    }

    /// Constructor
    /**
     * Constructs an empty leg allocating memory for a given number of segments.
     */
    leg_s(unsigned n_seg, double c, double alpha, int tol = -10)
        : m_ti(), m_xi(), m_throttles(n_seg), m_tf(), m_xf(), m_sf(0), m_sc(), m_mu(), m_c(c), m_alpha(alpha),
          m_tol(tol), m_states(n_seg + 2), m_ceq(), m_cineq(n_seg), m_dv(n_seg)
    {
    }

    /// Sets the leg's data
    /**
    * The throttles are provided via two iterators pointing to the beginning and
    to the end of
    * a sequence of doubles (\f$ x_1,y_1,z_1, ..., x_N,y_N,z_N \f$ containing the
    * cartesian components of each throttle \f$ x_i,y_i,z_i \in [0,1]\f$. The
    constructed leg
    * will have equally spaced segments in the pseudo-time
    *
    * \param[in] epoch_i Inital epoch
    * \param[in] state_i Initial sc_state (spacecraft state)
    * \param[in] throttles_start iterator pointing to the beginning of a cartesian
    throttle sequence.
    * \param[in] throttles_end iterator pointing to the end+1 of a cartesian
    throttle sequence.
    * \param[in] epoch_f Final epoch. Needs to be later than epoch_i
    * \param[in] state_f Final sc_state (spacecraft state)
    * \param[in] s_f Pseudo time of transfer
    * \param[in] mu_ Primary body gravitational constant
    * \param[in] sc_ Spacecraft
    *
    & \throws value_error if final epoch is before initial epoch, if mu_ not
    positive if the throttle size is not consistent
    */
    template <typename it_type>
    void
    set_leg(const epoch &epoch_i, const sc_state &state_i, it_type throttles_start, it_type throttles_end,
            const epoch &epoch_f, const sc_state &state_f, const double &sf, const spacecraft &sc_, const double &mu_,
            typename boost::enable_if<boost::is_same<typename std::iterator_traits<it_type>::value_type, double>>::type
                * = 0)
    {
        // We check data consistency
        if (std::distance(throttles_start, throttles_end) % 3) {
            throw_value_error("The length of the throttles list must be a multiple of 3");
        }
        if (std::distance(throttles_start, throttles_end) / 3 != (int)m_throttles.size()) {
            throw_value_error("The number of segments in the leg do not match the "
                              "length of the supplied throttle sequence");
        }
        if (epoch_i.mjd2000() >= epoch_f.mjd2000()) {
            throw_value_error("Final epoch must be strictly after initial epoch");
        }

        if (mu_ <= 0) {
            throw_value_error("Gravity parameter must be larger than zero (forgot to set it?)");
        }
        if (epoch_i.mjd() >= epoch_f.mjd()) {
            throw_value_error("Final epoch must be after the initial epoch");
        }
        if (sc_.get_mass() == 0) {
            throw_value_error("Spacecraft mass must be larger than zero (forgot to set it?)");
        }

        // We fill up all leg's data member
        m_mu = mu_;
        m_sc = sc_;
        m_ti = epoch_i;
        m_xi = state_i;
        m_tf = epoch_f;
        m_xf = state_f;
        m_sf = sf;

        // note: the epochs of the throttles are meaningless at this point as
        // pseudo-time is used
        for (decltype(m_throttles.size()) i = 0; i < m_throttles.size(); ++i) {
            kep_toolbox::array3D tmp
                = {{*(throttles_start + 3 * i), *(throttles_start + 3 * i + 1), *(throttles_start + 3 * i + 2)}};
            m_throttles[i] = throttle(epoch(0.), epoch(1.), tmp);
        }
    }

    /// Sets the leg's data
    void set_leg(const epoch &epoch_i, const sc_state &state_i, const std::vector<double> &thrott, const epoch &epoch_f,
                 const sc_state &state_f, const double &sf)
    {
        set_leg(epoch_i, state_i, thrott.begin(), thrott.end(), epoch_f, state_f, sf, m_sc, m_mu);
    }

    /// Sets the leg's data
    void set_leg(const epoch &epoch_i, const sc_state &state_i, const std::vector<double> &thrott, const epoch &epoch_f,
                 const sc_state &state_f, const double &sf, const spacecraft &sc_, const double &mu_)
    {
        set_leg(epoch_i, state_i, thrott.begin(), thrott.end(), epoch_f, state_f, sf, sc_, mu_);
    }

    /** @name Setters*/
    //@{

    /// Sets the leg's spacecraft
    /**
     *
     * In order for the trajectory leg to be able to propagate the states,
     * information on the
     * low-thrust propulsion system used needs to be available. This is provided by
     * the object
     * spacecraft private member of the class and can be set using this setter.
     *
     * \param[in] sc The spacecraft object
     */
    void set_sc(const spacecraft &sc)
    {
        m_sc = sc;
    }

    /// Sets the leg's primary body gravitational parameter
    /**
     *
     * Sets the leg's central body gravitational parameter
     *
     * \param[in] mu_ The gravitational parameter
     */
    void set_mu(const double &mu_)
    {
        m_mu = mu_;
    }

    /** @name Getters*/
    //@{

    /// Gets the leg's spacecraft
    /**
     * Returns the spacecraft
     *
     * @return sc const reference to spacecraft object
     */
    const spacecraft &get_spacecraft() const
    {
        return m_sc;
    }

    /// Gets the gravitational parameter
    /**
     * @return the gravitational parameter
     */
    double get_mu() const
    {
        return m_mu;
    }

    /// Gets the leg' s number of segments
    /**
     * Returns the leg' s number of segments
     *
     * @return size_t the leg's number of segments
     */
    size_t get_n_seg() const
    {
        return m_throttles.size();
    }

    /// Gets the leg's initial epoch
    /**
     * Gets the epoch at the beginning of the leg
     *
     * @return const reference to the initial epoch
     */
    const epoch &get_ti() const
    {
        return m_ti;
    }

    /// Gets the leg's final epoch
    /**
     * Gets the epoch at the end of the leg
     *
     * @return const reference to the final epoch
     */
    const epoch &get_tf() const
    {
        return m_tf;
    }

    /// Gets the sc_state at the end of the leg
    /**
     * Gets the spacecraft state at the end of the leg
     *
     * @return const reference to the final sc_state
     */
    const sc_state &get_xf() const
    {
        return m_xf;
    }

    /// Gets the initial sc_state
    /**
     * Gets the spacecraft state at the beginning of the leg
     *
     * @return const reference to the initial sc_state
     */
    const sc_state &get_xi() const
    {
        return m_xi;
    }
    //@}

    /** @name Computations*/
    //@{

    /// Returns the computed state mismatch constraints (8 equality constraints)
    /**
     * The main trajectory computations are made here. The method propagates the
     * leg's throttle sequence
     * to return the 8-mismatch constraints (leg with the Sundmann variable). For
     * efficiency purposes the method
     * does not store any of the computed intermediate steps. If those are also
     * needed use the slower method
     * compute_states.
     *
     * @return const reference to the m_ceq data member that contains, after the
     * call to this method, the 8 mismatches
     */

    const std::array<double, 8> &compute_mismatch_con() const
    {
        auto n_seg = m_throttles.size();
        auto n_seg_fwd = (n_seg + 1) / 2, n_seg_back = n_seg / 2;

        // Aux variables
        double max_thrust = m_sc.get_thrust();
        double veff = m_sc.get_isp() * ASTRO_G0;
        array3D thrust;
        double ds = m_sf / static_cast<double>(n_seg);                 // pseudo-time interval for each segment
        double dt = (m_tf.mjd2000() - m_ti.mjd2000()) * ASTRO_DAY2SEC; // length of the leg in seconds

        // Initial state
        array3D rfwd = m_xi.get_position();
        array3D vfwd = m_xi.get_velocity();
        double mfwd = m_xi.get_mass();
        double tfwd = 0;

        // Forward Propagation
        for (decltype(n_seg_fwd) i = 0u; i < n_seg_fwd; i++) {
            for (int j = 0; j < 3; j++) {
                thrust[j] = max_thrust * m_throttles[i].get_value()[j];
            }
            propagate_taylor_s(rfwd, vfwd, mfwd, tfwd, thrust, ds, m_mu, veff, m_c, m_alpha, m_tol, m_tol);
        }

        // Final state
        array3D rback = m_xf.get_position();
        array3D vback = m_xf.get_velocity();
        double mback = m_xf.get_mass();
        double tback = 0;

        // Backward Propagation
        for (decltype(n_seg_back) i = 0u; i < n_seg_back; ++i) {
            for (unsigned j = 0u; j < 3u; ++j) {
                thrust[j] = max_thrust * m_throttles[m_throttles.size() - i - 1].get_value()[j];
            }
            propagate_taylor_s(rback, vback, mback, tback, thrust, -ds, m_mu, veff, m_c, m_alpha, m_tol, m_tol);
        }

        // Return the mismatch
        diff(rfwd, rfwd, rback);
        diff(vfwd, vfwd, vback);

        std::copy(rfwd.begin(), rfwd.end(), m_ceq.begin());
        std::copy(vfwd.begin(), vfwd.end(), m_ceq.begin() + 3);
        m_ceq[6] = mfwd - mback;
        m_ceq[7] = (tfwd - tback - dt);
        return m_ceq;
    }
    /// Returns the computed throttles constraints (n_seg inequality constraints)
    /**
     * Computes the throttles constraints frm the throttle vector performing a
     * fairly straight forward computation
     *
     * @return const reference to the m_ceq data member that contains, after the
     * call to this method, the 8 mismatches
     */
    const std::vector<double> &compute_throttles_con() const
    {
        for (size_t i = 0; i < m_throttles.size(); ++i) {
            const array3D &t = m_throttles[i].get_value();
            m_cineq[i] = std::inner_product(t.begin(), t.end(), t.begin(), -1.);
        }
        return m_cineq;
    }
    /**
     * The main trajectory computations are made here. Less efficiently than in
     * compute_mismatch_con, but storing
     * more info on the way. The method propagates the leg's throttle sequence to
     * return the spacecraft state at each time.
     * The code is redundand with compute_mismatch_con, the choice to have it
     * redundand stems from the need to have
     * compute_mismatch_con method to be as efficient as possible, while
     * compute_states can also be unefficient.
     *
     * @return a (n_seg + 2) vector of 8-dim arrays containing t,x,y,z,vx,vy,vz,m
     */
    const std::vector<std::array<double, 11>> &compute_states() const
    {
        size_t n_seg = m_throttles.size();
        auto n_seg_fwd = (n_seg + 1) / 2, n_seg_back = n_seg / 2;

        // Aux variables
        double max_thrust = m_sc.get_thrust();
        double veff = m_sc.get_isp() * ASTRO_G0;
        array3D thrust = {{0, 0, 0}};
        array3D zeros = {{0, 0, 0}};
        double ds = m_sf / static_cast<double>(n_seg);                                      // pseudo-time interval for each segment
        double dt = (m_tf.mjd2000() - m_ti.mjd2000()) * ASTRO_DAY2SEC; // length of the leg in seconds

        // Initial state
        array3D rfwd = m_xi.get_position();
        array3D vfwd = m_xi.get_velocity();
        double mfwd = m_xi.get_mass();
        double tfwd = 0;

        record_states(tfwd, rfwd, vfwd, mfwd, zeros, 0);

        // Forward Propagation
        for (decltype(n_seg_fwd) i = 0u; i < n_seg_fwd; ++i) {
            for (unsigned j = 0u; j < 3u; ++j) {
                thrust[j] = max_thrust * m_throttles[i].get_value()[j];
            }
            try {
                propagate_taylor_s(rfwd, vfwd, mfwd, tfwd, thrust, ds, m_mu, veff, m_c, m_alpha, m_tol, m_tol);
            } catch (...) {
                throw_value_error("Could not compute the states ... check your data!!!");
            }
            record_states(tfwd, rfwd, vfwd, mfwd, thrust, i + 1);
        }

        // Final state
        array3D rback = m_xf.get_position();
        array3D vback = m_xf.get_velocity();
        double mback = m_xf.get_mass();
        double tback = 0;

        record_states(dt + tback, rback, vback, mback, zeros, n_seg + 1);

        // Backward Propagation
        for (decltype(n_seg_back) i = 0u; i < n_seg_back; ++i) {
            for (unsigned j = 0u; j < 3u; ++j) {
                thrust[j] = max_thrust * m_throttles[m_throttles.size() - i - 1].get_value()[j];
            }
            try {
                propagate_taylor_s(rback, vback, mback, tback, thrust, -ds, m_mu, veff, m_c, m_alpha, m_tol, m_tol);
            } catch (...) {
                throw_value_error("Could not compute the states ... check your data!!!");
            }
            record_states(dt + tback, rback, vback, mback, thrust, n_seg - i);
        }
        // Return the states
        return m_states;
    }

    // const std::vector<double>&  compute_dvs() const { return m_dv; }
    const std::vector<throttle> &get_throttles()
    {
        size_t n_seg = m_throttles.size();
        const size_t n_seg_fwd = (n_seg + 1) / 2;
        std::vector<std::array<double, 11>> states;
        states = compute_states();
        for (size_t i = 0; i < n_seg_fwd; ++i) {
            m_throttles[i].set_start(epoch(states[i][0] * ASTRO_SEC2DAY + m_ti.mjd2000()));
            m_throttles[i].set_end(epoch(states[i + 1][0] * ASTRO_SEC2DAY + m_ti.mjd2000()));
        }

        for (size_t i = n_seg_fwd; i < n_seg; ++i) {
            m_throttles[i].set_start(epoch(states[i + 1][0] * ASTRO_SEC2DAY + m_ti.mjd2000()));
            m_throttles[i].set_end(epoch(states[i + 2][0] * ASTRO_SEC2DAY + m_ti.mjd2000()));
        }
        return m_throttles;
    }
    //@}

protected:
    void record_states(double t, const array3D &r, const array3D &v, double m, const array3D &thrust,
                       size_t idx) const
    {
        assert((idx + 1) < m_states.size());
        m_states[idx][0] = t;
        std::copy(r.begin(), r.end(), m_states[idx].begin() + 1);
        std::copy(v.begin(), v.end(), m_states[idx].begin() + 4);
        m_states[idx][7] = m;
        std::copy(thrust.begin(), thrust.end(), m_states[idx].begin() + 8);
    }

private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int)
    {
        ar &m_ti;
        ar &m_xi;
        ar &m_throttles;
        ar &m_tf;
        ar &m_xf;
        ar &m_sf;
        ar &m_sc;
        ar &m_mu;
        ar &m_c;
        ar &m_alpha;
        ar &m_tol;
        ar &m_states;
        ar &m_ceq;
        ar &m_cineq;
        ar &m_dv;
    }

    epoch m_ti;
    sc_state m_xi;
    std::vector<throttle> m_throttles;
    epoch m_tf;
    sc_state m_xf;
    double m_sf;
    spacecraft m_sc;
    double m_mu;
    double m_c;
    double m_alpha;
    int m_tol;

    mutable std::vector<std::array<double, 11>> m_states;
    mutable std::array<double, 8> m_ceq;
    mutable std::vector<double> m_cineq;
    mutable std::vector<double> m_dv;
};


} // namespace sims_flanagan
} // namespace kep_toolbox
#endif // KEP_TOOLBOX_LEG_S_H
