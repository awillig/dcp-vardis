/**
 * Copyright (C) 2026 Andreas Willig, University of Canterbury
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */


#pragma once

#include <cstdint>
#include <string>
#include <format>
#include <stdexcept>

/**
 * @brief This module provides definitions of the SRP status names,
 *        and a helper function to convert an SRP status name
 */


namespace dcp::srp {

  /**
   * @brief SRP status names
   */


  enum class SRPStatus : std::uint16_t {
    Ok,
    Inactive
    };

};  // namespace dcp::srp



/**
 * @brief Converts SRP status name to string
 *
 * Throws for unknown status name
 */

constexpr std::string_view to_string(dcp::srp::SRPStatus stat) {
    switch (stat)
      {
	// status codes prescribed by the specification
      case dcp::srp::SRPStatus::Ok:         return "SRP-STATUS-OK";
      case dcp::srp::SRPStatus::Inactive:   return "SRP-STATUS-INACTIVE";
      default:
	throw std::invalid_argument(std::format("to_string: illegal SRP status code {}", static_cast<uint16_t> (stat)));
      }
    return "";
  }
    




std::ostream& operator<< (std::ostream& os, const dcp::srp::SRPStatus stat);
