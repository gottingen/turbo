// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <turbo/flags/cli/string_tools.h>
#include <turbo/flags/cli/error.h>

namespace turbo::cli {

    class App;

/// Holds values to load into Options
    struct ConfigItem {
        /// This is the list of parents
        std::vector<std::string> parents{};

        /// This is the name
        std::string name{};
        /// Listing of inputs
        std::vector<std::string> inputs{};

        /// The list of parents and name joined by "."
        [[nodiscard]] std::string fullname() const {
            std::vector<std::string> tmp = parents;
            tmp.emplace_back(name);
            return detail::join(tmp, ".");
        }
    };

/// This class provides a converter for configuration files.
    class Config {
    protected:
        std::vector<ConfigItem> items{};

    public:
        /// Convert an app into a configuration
        virtual std::string to_config(const App *, bool, bool, std::string) const = 0;

        /// Convert a configuration into an app
        virtual std::vector<ConfigItem> from_config(std::istream &) const = 0;

        /// Get a flag value
        [[nodiscard]] virtual std::string to_flag(const ConfigItem &item) const {
            if (item.inputs.size() == 1) {
                return item.inputs.at(0);
            }
            if (item.inputs.empty()) {
                return "{}";
            }
            throw ConversionError::TooManyInputsFlag(item.fullname());  // LCOV_EXCL_LINE
        }

        /// Parse a config file, throw an error (ParseError:ConfigParseError or FileError) on failure
        [[nodiscard]] std::vector<ConfigItem> from_file(const std::string &name) const {
            std::ifstream input{name};
            if (!input.good())
                throw FileError::Missing(name);

            return from_config(input);
        }

        /// Virtual destructor
        virtual ~Config() = default;
    };

/// This converter works with INI/TOML files; to write INI files use ConfigINI
    class ConfigBase : public Config {
    protected:
        /// the character used for comments
        char commentChar = '#';
        /// the character used to start an array '\0' is a default to not use
        char arrayStart = '[';
        /// the character used to end an array '\0' is a default to not use
        char arrayEnd = ']';
        /// the character used to separate elements in an array
        char arraySeparator = ',';
        /// the character used separate the name from the value
        char valueDelimiter = '=';
        /// the character to use around strings
        char stringQuote = '"';
        /// the character to use around single characters and literal strings
        char literalQuote = '\'';
        /// the maximum number of layers to allow
        uint8_t maximumLayers{255};
        /// the separator used to separator parent layers
        char parentSeparatorChar{'.'};
        /// Specify the configuration index to use for arrayed sections
        int16_t configIndex{-1};
        /// Specify the configuration section that should be used
        std::string configSection{};

    public:
        std::string
        to_config(const App * /*app*/, bool default_also, bool write_description, std::string prefix) const override;

        std::vector<ConfigItem> from_config(std::istream &input) const override;

        /// Specify the configuration for comment characters
        ConfigBase *comment(char cchar) {
            commentChar = cchar;
            return this;
        }

        /// Specify the start and end characters for an array
        ConfigBase *arrayBounds(char aStart, char aEnd) {
            arrayStart = aStart;
            arrayEnd = aEnd;
            return this;
        }

        /// Specify the delimiter character for an array
        ConfigBase *arrayDelimiter(char aSep) {
            arraySeparator = aSep;
            return this;
        }

        /// Specify the delimiter between a name and value
        ConfigBase *valueSeparator(char vSep) {
            valueDelimiter = vSep;
            return this;
        }

        /// Specify the quote characters used around strings and literal strings
        ConfigBase *quoteCharacter(char qString, char literalChar) {
            stringQuote = qString;
            literalQuote = literalChar;
            return this;
        }

        /// Specify the maximum number of parents
        ConfigBase *maxLayers(uint8_t layers) {
            maximumLayers = layers;
            return this;
        }

        /// Specify the separator to use for parent layers
        ConfigBase *parentSeparator(char sep) {
            parentSeparatorChar = sep;
            return this;
        }

        /// get a reference to the configuration section
        std::string &sectionRef() { return configSection; }

        /// get the section
        [[nodiscard]] const std::string &section() const { return configSection; }

        /// specify a particular section of the configuration file to use
        ConfigBase *section(const std::string &sectionName) {
            configSection = sectionName;
            return this;
        }

        /// get a reference to the configuration index
        int16_t &indexRef() { return configIndex; }

        /// get the section index
        [[nodiscard]] int16_t index() const { return configIndex; }

        /// specify a particular index in the section to use (-1) for all sections to use
        ConfigBase *index(int16_t sectionIndex) {
            configIndex = sectionIndex;
            return this;
        }
    };

    /// the default Config is the TOML file format
    using ConfigTOML = ConfigBase;

    /// ConfigINI generates a "standard" INI compliant output
    class ConfigINI : public ConfigTOML {

    public:
        ConfigINI() {
            commentChar = ';';
            arrayStart = '\0';
            arrayEnd = '\0';
            arraySeparator = ' ';
            valueDelimiter = '=';
        }
    };
}  // namespace turbo::cli
