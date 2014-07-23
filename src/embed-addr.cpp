/**
 * Copyright (c) 2011-2014 sx developers (see AUTHORS)
 *
 * This file is part of sx.
 *
 * sx is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <sx/command/embed-addr.hpp>

#include <iostream>
#include <sx/utility/console.hpp>

using namespace sx;
using namespace sx::extension;

console_result embed_addr::invoke(std::istream& input, std::ostream& output,
    std::ostream& cerr)
{
    // Bound parameters.
    auto data = get_data_argument();

    cerr << SX_EMBED_ADDR_NOT_IMPLEMENTED << std::endl;
    return console_result::failure;
}

//#!/bin/bash
//read INPUT
//DECODED_ADDR=$(echo $INPUT | sx ripemd-hash)
//SCRIPT=$(sx rawscript dup hash160 [ $DECODED_ADDR ] equalverify checksig)
//HASH=$(echo $SCRIPT | sx ripemd-hash)
//sx encode-addr $HASH
//
