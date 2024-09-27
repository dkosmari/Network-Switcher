/*
 * Network Switcher - A plugin to switch network profiles on the Wii U.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <atomic>
#include <stdexcept>

#include <notifications/notifications.h>

#include <wupsxx/logger.hpp>

#include "notify.hpp"


namespace logger = wups::logger;


namespace notify {

    std::atomic_uint refs = 0;
    std::string prefix;


    void
    initialize(const std::string& pre)
    {
        if (refs++)
            return;

        if (!pre.empty())
            prefix = "[" + pre + "] ";
        else
            prefix.clear();

        auto res = NotificationModule_InitLibrary();
        if (res != NOTIFICATION_MODULE_RESULT_SUCCESS) {
            logger::printf("Failed to initialize notifications: %s\n",
                           NotificationModule_GetStatusStr(res));
            return;
        }

        NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO,
                                           NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT,
                                           5.0f);
        NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_ERROR,
                                           NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT,
                                           5.0f);
    }


    void
    finalize()
    {
        if (--refs)
            return;

        prefix.clear();

        NotificationModule_DeInitLibrary();
    }


    void
    info(const std::string& msg)
        noexcept
    {
        if (!refs) {
            logger::printf("Forgot to initialize notifications library.\n");
            return;
        }

        try {
            std::string full_msg = prefix + msg;
            NotificationModule_AddInfoNotification(full_msg.c_str());
        }
        catch (std::exception& e) {
            logger::printf("Error: %s\n", e.what());
        }
    }


    void
    vinfof(const char* fmt, std::va_list args)
        noexcept
    {
        try {
            std::va_list args2;

            va_copy(args2, args);
            int size = std::vsnprintf(nullptr, 0, fmt, args2);
            va_end(args2);

            if (size < 0)
                throw std::runtime_error{"vsnprintf() failed\n"};

            std::string buf(size + 1, '\0');
            std::vsnprintf(buf.data(), buf.size(), fmt, args);
            info(buf);
        }
        catch (std::exception& e) {
            logger::printf("Error: %s\n", e.what());
        }
    }


    void
    infof(const char* fmt, ...)
        noexcept
    {
        std::va_list args;
        va_start(args, fmt);
        vinfof(fmt, args);
        va_end(args);
    }


    void
    error(const std::string& msg)
        noexcept
    {
        if (!refs) {
            logger::printf("Forgot to initialize notifications library.\n");
            return;
        }

        try {
            std::string full_msg = prefix + msg;
            NotificationModule_AddErrorNotification(full_msg.c_str());
        }
        catch (std::exception& e) {
            logger::printf("Error: %s\n", e.what());
        }
    }


    void
    verrorf(const char* fmt, std::va_list args)
        noexcept
    {
        try {
            std::va_list args2;

            va_copy(args2, args);
            int size = std::vsnprintf(nullptr, 0, fmt, args2);
            va_end(args2);

            if (size < 0)
                throw std::runtime_error{"vsnprintf() failed\n"};

            std::string buf(size + 1, '\0');
            std::vsnprintf(buf.data(), buf.size(), fmt, args);
            error(buf);
        }
        catch (std::exception& e) {
            logger::printf("Error: %s\n", e.what());
        }
    }


    void
    errorf(const char* fmt, ...)
        noexcept
    {
        std::va_list args;
        va_start(args, fmt);
        verrorf(fmt, args);
        va_end(args);
    }

} // namespace notify
