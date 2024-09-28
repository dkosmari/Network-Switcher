/*
 * Network Switcher - A plugin to switch network profiles on the Wii U.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <future>
#include <memory>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <utility>

#include <nn/ac.h>
#include <nsysnet/netconfig.h>

#include <wups.h>

#include <wupsxx/button_item.hpp>
#include <wupsxx/category.hpp>
#include <wupsxx/init.hpp>
#include <wupsxx/int_item.hpp>
#include <wupsxx/logger.hpp>

#include "notify.hpp"
#include "wut_extras.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using std::runtime_error;

namespace logger = wups::logger;


WUPS_PLUGIN_NAME(PACKAGE_NAME);
WUPS_PLUGIN_VERSION(PACKAGE_VERSION);
WUPS_PLUGIN_DESCRIPTION("Switch between network profiles.");
WUPS_PLUGIN_AUTHOR("Daniel K. O.");
WUPS_PLUGIN_LICENSE("GPLv3");


int startup_id = 0;
int compat_id = 0;


struct nn_ac_guard {

    nn_ac_guard()
    {
        if (!nn::ac::Initialize())
            throw runtime_error{"nn::ac::Initialize() failed"};
    }

    ~nn_ac_guard()
    {
        nn::ac::Finalize();
    }

};


std::string
get_ssid(const NetConfWifiConfigData& cfg)
{
    std::size_t len = std::min<std::size_t>(cfg.ssidlength, sizeof cfg.ssid);
    return std::string(reinterpret_cast<const char*>(cfg.ssid),
                       len);
}


struct net_profile_item : wups::config::button_item {

    const nn::ac::ConfigIdNum id;
    std::string description;
    std::future<void> task_result;
    std::stop_source task_stopper;


    net_profile_item(nn::ac::ConfigIdNum id) :
        button_item{"Profile " + std::to_string(id)},
        id{id}
    {
        using std::to_string;

        bool valid = false;
        nn::ac::IsConfigExisting(id, &valid);

        if (!valid) {
            status_msg = "<empty>";
        } else {
            nn::ac::Config cfg;
            if (!nn::ac::ReadConfig(id, &cfg)) {
                status_msg = "Error!";
                return;
            }

            if (cfg.wl0.if_sate)
                description = "[Wi-Fi] SSID=\"" + get_ssid(cfg.wifi.config) + "\"";
            else if (cfg.eth0.if_sate)
                description = "[Ethernet]";

            status_msg = description;
        }
    }


    static
    std::unique_ptr<net_profile_item>
    create(nn::ac::ConfigIdNum id)
    {
        return std::make_unique<net_profile_item>(id);
    }


    virtual
    void
    on_started()
        override
    {
        status_msg = "Connecting...";

        task_stopper = {};

        auto task = [this](std::stop_token token)
        {
            try {
                nn_ac_guard guard;

                if (token.stop_requested())
                    throw runtime_error{"Canceled by user"};

                nn::ac::CloseAll();

                if (token.stop_requested())
                    throw runtime_error{"Canceled by user"};

                if (!nn::ac::Connect(id))
                    throw runtime_error{"Failed to switch to profile " + std::to_string(id)};

                current_state = state::stopped;
            }
            catch (std::exception& e) {
                current_state = state::stopped;
                throw;
            }
        };

        task_result = std::async(std::launch::async,
                                 std::move(task),
                                 task_stopper.get_token());
    }


    virtual
    void
    on_cancel()
        override
    {
        task_stopper.request_stop();
    }


    virtual
    void
    on_finished()
        override
    {
        try {
            task_result.get();
            status_msg = "Done! " + description;
            notify::infof("Using profile %u: %s",
                          static_cast<unsigned>(id),
                          description.c_str());
        }
        catch (std::exception& e) {
            notify::errorf("Error: %s", e.what());
            logger::printf("Error: %s\n", e.what());
            status_msg = e.what();
        }
    }

};


struct disconnect_item : wups::config::button_item {

    disconnect_item() :
        button_item{"Disconnect"}
    {}


    static
    std::unique_ptr<disconnect_item>
    create()
    {
        return std::make_unique<disconnect_item>();
    }


    virtual
    void
    on_started()
        override
    {
        nn_ac_guard guard;

        if (nn::ac::CloseAll())
            status_msg = "Done!";
        else
            status_msg = "Error!";

        current_state = state::stopped;
    }

};


void
menu_open(wups::config::category& root)
{
    using wups::config::int_item;

    logger::initialize(PACKAGE_NAME);
    notify::initialize(PACKAGE_NAME);

    nn_ac_guard guard;

    for (nn::ac::ConfigIdNum id = 1; id <= 6; ++id)
        root.add(net_profile_item::create(id));

    nn::ac::ConfigIdNum sid = 0;
    if (nn::ac::GetStartupId(&sid))
        startup_id = sid;
    root.add(int_item::create("Default profile",
                              startup_id,
                              startup_id,
                              1, 6));

    nn::ac::ConfigIdNum cid = 0;
    if (nn::ac::GetCompatId(&cid))
        compat_id = cid;
    root.add(int_item::create("vWii profile",
                              compat_id,
                              compat_id,
                              1, 6));

    root.add(disconnect_item::create());
}


void
menu_close()
{
    nn_ac_guard guard;

    nn::ac::ConfigIdNum sid = 0;
    if (nn::ac::GetStartupId(&sid) && static_cast<int>(sid) != startup_id) {
        if (nn::ac::SetStartupId(startup_id)) {
            notify::infof("Set default profile to %d", startup_id);
        } else {
            notify::errorf("Could not set default profile to %d", startup_id);
            logger::printf("nn::ac::SetStartupId(%d) failed\n", startup_id);
        }
    }

    nn::ac::ConfigIdNum cid = 0;
    if (nn::ac::GetCompatId(&cid) && static_cast<int>(cid) != compat_id) {
        if (nn::ac::SetCompatId(compat_id)) {
            notify::infof("Set vWii profile to %d", compat_id);
        } else {
            notify::errorf("Could not set vWii profile to %d", compat_id);
            logger::printf("nn::ac::SetCompatId(%d) failed\n", compat_id);
        }
    }

    notify::finalize();
    logger::finalize();
}


INITIALIZE_PLUGIN()
{
    logger::guard guard{PACKAGE_NAME};

    try {
        wups::config::init(PACKAGE_NAME, menu_open, menu_close);
    }
    catch (std::exception& e) {
        logger::printf("ERROR: %s\n", e.what());
    }
}
