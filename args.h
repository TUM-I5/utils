// SPDX-FileCopyrightText: 2013-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger

#ifndef UTILS_ARGS_H_
#define UTILS_ARGS_H_

#include "utils/stringutils.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// NOLINTNEXTLINE
#include <getopt.h>

namespace utils {

/**
 * Parses command line arguments
 * @todo comment functions
 */
class Args {
  private:
  struct OptionInfo {
    /** Leave empty if this not an enum option */
    std::vector<std::string> enumValues;
    std::string longOption; // We need a copy here to get the const char* correct
    /** Name of the value in the help command */
    std::string value;
    std::string description;
    bool required;
  };

  struct AdditionalOptionInfo {
    std::string name;
    std::string description;
    bool required;
  };

  /**
   * Convert a long option into an "argument" that is shown in the help message
   */
  struct ValueConvert {
    void operator()(char& c) {
      c = toupper(static_cast<unsigned char>(c));
      switch (c) {
      case '-':
        c = '_';
        break;
      }
    }
  };

  /** Program description (can be empty) */
  const std::string mDescription;
  /** Automatically add help option */
  const bool mAddHelp;

  /** The command line arguments */
  std::vector<struct option> m_options;
  /**
   * Additional information for the command line arguments
   * required to generate help information
   */
  std::vector<OptionInfo> m_optionInfo;

  std::vector<AdditionalOptionInfo> m_additionalOptionInfo;

  /** Maps from short option to index in m_options */
  std::unordered_map<char, size_t> m_short2option;

  /** Contains the arguments after parse was called */
  std::unordered_map<std::string, std::string> m_arguments;

  /**
   * Contains additional arguments after parse was called
   * @todo Find a better name
   */
  std::unordered_map<std::string, std::string> m_additionalArguments;

  /** Additional user-defined help message */
  std::string m_customHelpMessage;

  public:
  enum Argument { Required = required_argument, No = no_argument, Optional = optional_argument };

  enum Result {
    Success = 0,
    Error,
    /** Help message printed */
    Help
  };

  Args(std::string description, bool addHelp = true)
      : mDescription(std::move(description)), mAddHelp(addHelp) {}

  void addOption(const std::string& longOption,
                 char shortOption,
                 const std::string& description,
                 Argument argument = Required,
                 bool required = true) {
    addOptionInternal(longOption, shortOption, description, argument, required);
  }

  /**
   * @param enumValues Use enumStart() and enumEnd() for easy char array
   * conversion
   */
  template <size_t N>
  [[deprecated]] void addEnumOption(const std::string& longOption,
                                    const char* (&enumValues)[N],
                                    char shortOption,
                                    const std::string& description,
                                    bool required = true) {
    std::vector<std::string> values(enumValues, end(enumValues));

    const std::string value = "{" + StringUtils::join(values, "|") + "}";

    addOptionInternal(longOption, shortOption, description, Required, required, value, values);
  }

  void addEnumOption(const std::string& longOption,
                     const std::vector<std::string>& values,
                     char shortOption,
                     const std::string& description,
                     bool required = true) {
    const std::string value = "{" + StringUtils::join(values, "|") + "}";

    addOptionInternal(longOption, shortOption, description, Required, required, value, values);
  }

  void addAdditionalOption(const std::string& name,
                           const std::string& description,
                           bool required = true) {
    if (!m_additionalOptionInfo.empty()) {
      if (required && !m_additionalOptionInfo.back().required) {
        // After one optional argument there can only be more optional arguments
        return;
      }
    }

    struct AdditionalOptionInfo const i = {name, description, required};
    m_additionalOptionInfo.push_back(i);
  }

  /**
   * Set a help message that is added to the parameter explanation
   */
  void setCustomHelpMessage(const std::string& message) { m_customHelpMessage = message; }

  /**
   * @return True of options are successfully parsed, false otherwise
   */
  auto parse(int argc, char* const* argv, bool printHelp = true) -> Result {
    if (mAddHelp) {
      addOption("help", 'h', "Show this help message", No, false);
    }

    std::ostringstream shortOptions;
    for (const auto& option : m_options) {
      if (option.val != 0) {
        shortOptions << static_cast<char>(option.val);
        switch (option.has_arg) {
        case required_argument:
          shortOptions << ':';
          break;
        case optional_argument:
          shortOptions << "::";
          break;
        }
      }
    }

    // Add null option
    struct option const o = {nullptr, 0, nullptr, 0};
    m_options.push_back(o);

    // Update const char* in m_options
    for (size_t i = 0; i < m_optionInfo.size(); i++) {
      m_options[i].name = m_optionInfo[i].longOption.c_str();
    }

    while (true) {
      int optionIndex = 0;

      const int c =
          getopt_long(argc, argv, shortOptions.str().c_str(), m_options.data(), &optionIndex);

      if (c < 0) {
        break;
      }

      switch (c) {
      case '?':
        if (printHelp) {
          helpMessage(argv[0], std::cerr);
        }
        return Error;
      case 0:
        // Nothing to do
        break;
      default:
        optionIndex = m_short2option.at(c);
      }

      if (optarg == nullptr) {
        m_arguments[m_options[optionIndex].name] = "";
      } else {
        m_arguments[m_options[optionIndex].name] = optarg;
      }

      if (!m_optionInfo[optionIndex].enumValues.empty()) {
        const auto i = std::find(m_optionInfo[optionIndex].enumValues.begin(),
                                 m_optionInfo[optionIndex].enumValues.end(),
                                 m_arguments[m_options[optionIndex].name]);
        if (i == m_optionInfo[optionIndex].enumValues.end()) {
          if (printHelp) {
            std::cerr << argv[0] << ": option --" << m_options[optionIndex].name
                      << " must be set to " << m_optionInfo[optionIndex].value << '\n';
            helpMessage(argv[0], std::cerr);
          }
          return Error;
        }

        m_arguments[m_options[optionIndex].name] =
            StringUtils::toString(i - m_optionInfo[optionIndex].enumValues.begin());
      }
    }

    if (mAddHelp && isSet("help")) {
      if (printHelp) {
        helpMessage(argv[0]);
      }
      return Help;
    }

    for (const auto& info : m_optionInfo) {
      if (info.required && !isSet(info.longOption)) {
        if (printHelp) {
          std::cerr << argv[0] << ": option --" << info.longOption << " is required" << '\n';
          helpMessage(argv[0], std::cerr);
        }
        return Error;
      }
    }

    // Parse additional options and check if all required options are set
    int i = 0;
    for (i = 0; i < argc - optind; i++) {
      if (i >= static_cast<int>(m_additionalOptionInfo.size())) {
        if (printHelp) {
          std::cerr << argv[0] << ": ignoring unknown parameter \"" << argv[i + optind] << "\""
                    << '\n';
        }
      } else {
        m_additionalArguments[m_additionalOptionInfo[i].name] = argv[i + optind];
      }
    }
    if (static_cast<size_t>(i) < m_additionalOptionInfo.size()) {
      if (m_additionalOptionInfo[i].required) {
        if (printHelp) {
          std::cerr << argv[0] << ": option <" << m_additionalOptionInfo[i].name << "> is required"
                    << '\n';
          helpMessage(argv[0], std::cerr);
        }
        return Error;
      }
    }

    return Success;
  }

  auto isSet(const std::string& option) const -> bool {
    return m_arguments.find(option) != m_arguments.end();
  }

  auto isSetAdditional(const std::string& option) const -> bool {
    return m_additionalArguments.find(option) != m_additionalArguments.end();
  }

  template <typename T>
  auto getArgument(const std::string& option) -> T {
    return StringUtils::parse<T>(m_arguments.at(option));
  }

  template <typename T>
  auto getArgument(const std::string& option, T defaultArgument) -> T {
    if (!isSet(option)) {
      return defaultArgument;
    }

    return getArgument<T>(option);
  }

  template <typename T>
  auto getAdditionalArgument(const std::string& option) -> T {
    return StringUtils::parse<T>(m_additionalArguments.at(option));
  }

  template <typename T>
  auto getAdditionalArgument(const std::string& option, T defaultArgument) -> T {
    if (!isSetAdditional(option)) {
      return defaultArgument;
    }

    return getAdditionalArgument<T>(option);
  }

  void helpMessage(const char* prog, std::ostream& out = std::cout) {
    // First line with all short options
    out << "Usage: " << prog;
    for (size_t i = 0; i < m_options.size() - 1; i++) {
      out << ' ';

      if (!m_optionInfo[i].required) {
        out << '[';
      }

      if (m_options[i].val != 0) {
        out << '-' << static_cast<char>(m_options[i].val);
      } else {
        out << "--" << m_options[i].name;
      }

      argumentInfo(i, out);

      if (!m_optionInfo[i].required) {
        out << ']';
      }
    }
    for (auto& i : m_additionalOptionInfo) {
      out << ' ';

      if (!i.required) {
        out << '[';
      }

      out << '<' << i.name << '>';

      if (!i.required) {
        out << ']';
      }
    }
    out << '\n';

    // General program description
    if (!mDescription.empty()) {
      out << '\n' << mDescription << '\n';
    }

    // Arguments
    if (!m_additionalOptionInfo.empty()) {
      out << '\n' << "arguments:" << '\n';
      for (auto& i : m_additionalOptionInfo) {
        out << "  <" << i.name << '>';

        // Number of characters used for the option
        const size_t length = 4 + i.name.size();

        if (length >= 30) {
          out << '\n';
          out << std::setw(30) << ' ';
        } else {
          out << std::setw(30 - length) << ' ';
        }

        out << i.description << '\n';
      }
    }

    // Optional arguments
    if (m_options.size() > 1) {
      out << '\n' << "optional arguments:" << '\n';
      for (size_t i = 0; i < m_options.size() - 1; i++) {
        out << "  ";

        // Number of characters used for the option
        size_t length = 2;

        if (m_options[i].val != 0) {
          out << '-' << static_cast<char>(m_options[i].val);
          out << ", ";
          length += 4;
        }

        out << "--" << m_options[i].name;
        length += m_optionInfo[i].longOption.size() + 2;
        length += argumentInfo(i, out);

        if (length >= 30) {
          out << '\n';
          out << std::setw(30) << ' ';
        } else {
          out << std::setw(30 - length) << ' ';
        }

        out << m_optionInfo[i].description << '\n';
      }
    }

    out << m_customHelpMessage;
  }

  private:
  void addOptionInternal(const std::string& longOption,
                         char shortOption,
                         const std::string& description,
                         Argument argument,
                         bool required,
                         const std::string& value = "",
                         const std::vector<std::string>& enumValues = std::vector<std::string>()) {

    if (shortOption != 0) {
      m_short2option[shortOption] = m_options.size();
    }

    std::string v;
    if (!value.empty()) {
      v = value;
    } else if (argument != No) {
      v = longOption;
      std::for_each(v.begin(), v.end(), ValueConvert());
    }

    struct OptionInfo const i = {enumValues, longOption, v, description, required};
    m_optionInfo.push_back(i);

    struct option const o = {
        m_optionInfo.back().longOption.c_str(), argument, nullptr, shortOption};
    m_options.push_back(o);
  }

  /**
   * Writes the argument information to out
   *
   * @param i The index of the option for which the argument should be generated
   * @return The number if characters written
   */
  auto argumentInfo(size_t i, std::ostream& out) -> size_t {
    switch (m_options[i].has_arg) {
    case required_argument:
      out << ' ' << m_optionInfo[i].value;
      return m_optionInfo[i].value.size() + 1;
    case optional_argument:
      out << " [" << m_optionInfo[i].value << ']';
      return m_optionInfo[i].value.size() + 3;
    }

    return 0;
  }

  template <typename T, size_t N>
  static auto end(T (&a)[N]) -> T* {
    return a + N;
  }
};

template <>
inline auto utils::Args::getArgument(const std::string& option, bool defaultArgument) -> bool {
  if (!isSet(option)) {
    return defaultArgument;
  }

  return !defaultArgument;
}

} // namespace utils

#endif // UTILS_ARGS_H_
