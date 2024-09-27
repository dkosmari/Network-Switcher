/*
 * Network Switcher - A plugin to switch network profiles on the Wii U.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NOTIFY_HPP
#define NOTIFY_HPP

#include <cstdarg>
#include <string>


namespace notify {

    void initialize(const std::string& prefix = "");

    void finalize();


    void info(const std::string& msg) noexcept;

    void vinfof(const char* fmt, std::va_list args) noexcept;

    __attribute__(( __format__ (__printf__, 1, 2)))
    void infof(const char* fmt, ...) noexcept;


    void error(const std::string& msg) noexcept;

    void verrorf(const char* fmt, std::va_list args) noexcept;

    void errorf(const char* fmt, ...) noexcept;

} // namespace notify

#endif
