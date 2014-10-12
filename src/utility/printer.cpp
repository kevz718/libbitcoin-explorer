/**
 * Copyright (c) 2011-2014 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin-explorer.
 *
 * libbitcoin-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
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

#include <bitcoin/explorer/utility/printer.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <bitcoin/explorer/define.hpp>

// We are doing this because po::options_description.print() sucks.

// TODO: move these to metadata for localization.
#define BX_PRINTER_USAGE_FORMAT "\nUsage: %1% %2% %3%\n"
#define BX_PRINTER_DESCRIPTION_FORMAT "\n%1%\n"
#define BX_PRINTER_CATEGORY_FORMAT "\nCategery: %1%\n"
#define BX_PRINTER_OPTION_TABLE_HEADER "\nOptions (named):\n\n"
#define BX_PRINTER_ARGUMENT_TABLE_HEADER "\nArguments (positional):\n\n"
#define BX_PRINTER_VALUE_TEXT "VALUE"

// Not localizable formatters.
#define BX_PRINTER_USAGE_OPTION_TABLE_FORMAT "-%1% [--%2%]"
#define BX_PRINTER_USAGE_OPTION_LONG_TABLE_FORMAT "--%1%"
#define BX_PRINTER_USAGE_OPTION_SHORT_TABLE_FORMAT "-%1%"
#define BX_PRINTER_USAGE_ARGUMENT_TABLE_FORMAT "%1%"

#define BX_PRINTER_USAGE_OPTION_TOGGLE_FORMAT " [-%1%]"
#define BX_PRINTER_USAGE_OPTION_MULTIPLE_FORMAT " [--%1% %2%]..."
#define BX_PRINTER_USAGE_OPTION_REQUIRED_FORMAT " --%1% %2%"
#define BX_PRINTER_USAGE_OPTION_OPTIONAL_FORMAT " [--%1% %2%]"

#define BX_PRINTER_USAGE_ARGUMENT_MULTIPLE_FORMAT " [%1%]..."
#define BX_PRINTER_USAGE_ARGUMENT_REQUIRED_FORMAT " %1%"
#define BX_PRINTER_USAGE_ARGUMENT_OPTIONAL_FORMAT " [%1%]"

using namespace bc::explorer;

printer::printer(const std::string& application, const std::string& category,
    const std::string& command, const std::string& description,
    const arguments_metadata& arguments, const options_metadata& options)
  : application_(application), category_(category), command_(command),
    description_(description), arguments_(arguments), options_(options)
{
}

/* Formatters */

// 100% component tested.
static void enqueue_fragment(std::string& fragment,
    std::vector<std::string>& column)
{
    trim(fragment);
    if (!fragment.empty())
        column.push_back(fragment);
}

// 100% component tested.
std::vector<std::string> printer::columnize(const std::string& paragraph,
    size_t width)
{
    const auto words = split(paragraph);

    std::string fragment;
    std::vector<std::string> column;

    for (const auto& word: words)
    {
        if (word.length() + fragment.length() < width)
        {
            fragment += " " + word;
            continue;
        }

        enqueue_fragment(fragment, column);
        fragment = word;
    }

    enqueue_fragment(fragment, column);
    return column;
}

// 100% component tested.
static format format_row_name(const parameter& value)
{
    if (value.get_position() != parameter::not_positional)
        return format(BX_PRINTER_USAGE_ARGUMENT_TABLE_FORMAT) %
            value.get_long_name();
    else if (value.get_short_name() == parameter::no_short_name)
        return format(BX_PRINTER_USAGE_OPTION_LONG_TABLE_FORMAT) %
        value.get_long_name();
    else if (value.get_long_name().empty())
        return format(BX_PRINTER_USAGE_OPTION_SHORT_TABLE_FORMAT) %
            value.get_short_name();
    else
        return format(BX_PRINTER_USAGE_OPTION_TABLE_FORMAT) %
            value.get_short_name() % value.get_long_name();
}

// 100% component tested.
static bool match_positional(bool positional, const parameter& value)
{
    auto positioned = value.get_position() != parameter::not_positional;
    return positioned == positional;
}

// 100% component tested.
// This formats to 80 char width as: [ 20 | ' ' | 58 | '\n' ]
std::string printer::format_parameters_table(bool positional)
{
    std::stringstream output;
    const auto& parameters = get_parameters();
    format table_format("%-20s %-58s\n");

    for (const auto& parameter: parameters)
    {
        // Skip positional arguments if not positional.
        if (!match_positional(positional, parameter))
            continue;

        // Get the formatted parameter name.
        auto name = format_row_name(parameter).str();

        // Build a column for the description.
        const auto rows = columnize(parameter.get_description(), 58);

        // If there is no description the command is not output!
        for (const auto& row: rows)
        {
            output << table_format % name % row;

            // The name is only set in the first row.
            name.clear();
        }
    }

    return output.str();
}

std::string printer::format_usage()
{
    // USAGE: bx COMMAND [-hvt] -n VALUE [-m VALUE] [-w VALUE]... REQUIRED 
    // [OPTIONAL] [MULTIPLE]...
    auto usage = format(BX_PRINTER_USAGE_FORMAT) % get_application() %
        get_command() % format_usage_parameters();
    return usage.str();
}

std::string printer::format_category()
{
    // CATEGORY: %1%\n
    auto category = format(BX_PRINTER_CATEGORY_FORMAT) % get_category();
    return category.str();
}

std::string printer::format_description()
{
    // DESCRIPTION: %1%\n
    auto description = format(BX_PRINTER_DESCRIPTION_FORMAT) % get_description();
    return description.str();
}

std::string printer::format_usage_parameters()
{
    std::string toggle_options;
    std::vector<std::string> required_options;
    std::vector<std::string> optional_options;
    std::vector<std::string> multiple_options;
    std::vector<std::string> required_arguments;
    std::vector<std::string> optional_arguments;
    std::vector<std::string> multiple_arguments;

    std::stringstream output;
    const auto& parameters = get_parameters();
    
    for (const auto& parameter: parameters)
    {
        // A required argument may only be preceeded by required arguments.
        // Requiredness may be in error if the metadata is inconsistent.
        auto required = parameter.get_required();

        // Options are named and args are positional.
        auto option = parameter.get_position() == parameter::not_positional;

        // In terms of formatting we treat all multivalued as not required.
        auto multiple = parameter.get_args_limit() > 1;

        // This will capture only options with zero_tokens() specified.
        auto toggle = parameter.get_args_limit() == 0;

        if (toggle)
        {
            toggle_options.push_back(parameter.get_short_name());
            continue;
        }

        const auto& long_name = parameter.get_long_name();

        if (option)
        {
            if (multiple)
                multiple_options.push_back(long_name);
            else if (required)
                required_options.push_back(long_name);
            else
                optional_options.push_back(long_name);
        }
        else
        {
            if (multiple)
                multiple_arguments.push_back(long_name);
            else if (required)
                required_arguments.push_back(long_name);
            else
                optional_arguments.push_back(long_name);
        }
    }

    std::stringstream usage;

    if (!toggle_options.empty())
        usage << format(BX_PRINTER_USAGE_OPTION_TOGGLE_FORMAT) % 
            toggle_options;

    for (const auto& required_option: required_options)
        usage << format(BX_PRINTER_USAGE_OPTION_REQUIRED_FORMAT) % 
            required_option % BX_PRINTER_VALUE_TEXT;

    for (const auto& multiple_option: multiple_options)
        usage << format(BX_PRINTER_USAGE_OPTION_MULTIPLE_FORMAT) % 
            multiple_option % BX_PRINTER_VALUE_TEXT;

    for (const auto& optional_option: optional_options)
        usage << format(BX_PRINTER_USAGE_OPTION_OPTIONAL_FORMAT) % 
            optional_option % BX_PRINTER_VALUE_TEXT;

    for (const auto& required_argument: required_arguments)
        usage << format(BX_PRINTER_USAGE_ARGUMENT_REQUIRED_FORMAT) % 
            required_argument;

    for (const auto& multiple_argument: multiple_arguments)
        usage << format(BX_PRINTER_USAGE_ARGUMENT_MULTIPLE_FORMAT) %
            multiple_argument;

    for (const auto& optional_argument: optional_arguments)
        usage << format(BX_PRINTER_USAGE_ARGUMENT_OPTIONAL_FORMAT) %
            optional_argument;

    std::string clean_usage(usage.str());
    trim(clean_usage);
    return clean_usage;
}

/* Initialization */

// 100% component tested.
static void enqueue_name(int count, std::string& name, argument_list& names)
{
    if (count <= 0)
        return;

    if (count > printer::max_arguments)
        count = -1;

    names.push_back(argument_pair(name, count));
}

// 100% component tested.
// This method just gives us a copy of arguments_metadata private state.
// It would be nice if instead that state was public.
void printer::generate_argument_names()
{
    // Member values
    const auto& arguments = get_arguments();
    auto& argument_names = get_argument_names();

    argument_names.clear();
    const auto max_total_arguments = arguments.max_total_count();

    // Temporary values
    std::string argument_name;
    std::string previous_argument_name;
    int max_previous_argument = 0;

    // We must enumerate all arguments to get the full set of names and counts.
    for (unsigned int position = 0; position < max_total_arguments && 
        max_previous_argument <= max_arguments; ++position)
    {
        argument_name = arguments.name_for_position(position);

        // Initialize the first name as having a zeroth instance.
        if (max_previous_argument == 0)
            previous_argument_name = argument_name;

        // This is a duplicate of the previous name, so increment the count.
        if (argument_name == previous_argument_name)
        {
            ++max_previous_argument;
            continue;
        }

        enqueue_name(max_previous_argument, previous_argument_name,
            argument_names);
        previous_argument_name = argument_name;
        max_previous_argument = 1;
    }

    // Save the previous name (if there is one).
    enqueue_name(max_previous_argument, previous_argument_name,
        argument_names);
}

// 100% component tested.
void printer::generate_parameters()
{
    const auto& argument_names = get_argument_names();
    const auto& options = get_options();
    auto& parameters = get_parameters();

    parameters.clear();

    parameter param;
    for (auto option_ptr: options.options())
    {
        param.initialize(*option_ptr, argument_names);
        parameters.push_back(param);
    }
}

// 100% component tested.
void printer::initialize()
{
    generate_argument_names();
    generate_parameters();
}

/* Printers */

void printer::print(std::ostream& output)
{
    output
        << format_usage()
        << format_description()
        << BX_PRINTER_OPTION_TABLE_HEADER
        << format_parameters_table(false)
        << BX_PRINTER_ARGUMENT_TABLE_HEADER
        << format_parameters_table(true);
}