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

#ifndef KEP_TOOLBOX_PLANET_JPL_LP_H
#define KEP_TOOLBOX_PLANET_JPL_LP_H

#include <keplerian_toolbox/detail/visibility.hpp>
#include <keplerian_toolbox/planet/base.hpp>
#include <keplerian_toolbox/serialization.hpp>

namespace kep_toolbox
{
namespace planet
{

/// Solar System Planet (jpl simplified ephemerides)
/**
 * This class allows to instantiate planets of
 * the solar system by referring to their common names. The ephemeris used
 * are low_precision ephemeris taken from http://ssd.jpl.nasa.gov/txt/p_elem_t1.txt
 * valid in the timeframe 1800AD - 2050 AD
 *
 * @author Dario Izzo (dario.izzo _AT_ googlemail.com)
 */

class KEP_TOOLBOX_DLL_PUBLIC jpl_lp : public base
{
public:
    jpl_lp(const std::string & = "earth");
    planet_ptr clone() const override;
    std::string human_readable_extra() const override;

private:
    void eph_impl(double mjd2000, array3D &r, array3D &v) const override;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int)
    {
        ar &boost::serialization::base_object<base>(*this);
        ar &jpl_elements;
        ar &jpl_elements_dot;
        ar &const_cast<double &>(ref_mjd2000);
    }

    array6D jpl_elements;
    array6D jpl_elements_dot;
    const double ref_mjd2000;
};
} // namespace planet
} // namespace kep_toolbox

BOOST_CLASS_EXPORT_KEY(kep_toolbox::planet::jpl_lp)

#endif // KEP_TOOLBOX_PLANET_JPL_LP_H
