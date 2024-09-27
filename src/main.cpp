/*
 * Network Switcher - A plugin to switch network profiles on the Wii U.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

#include <nn/ac.h>
#include <nsysnet/netconfig.h>

#include <wups.h>

#include <wupsxx/button_item.hpp>
#include <wupsxx/category.hpp>
#include <wupsxx/init.hpp>
#include <wupsxx/int_item.hpp>
#include <wupsxx/logger.hpp>

#include "notify.hpp"

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


int startup_id = 1;


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


// fill-in missing parts of wut
namespace nn::ac {

    using Config = NetConfCfg;

    namespace detail {

        extern "C" {

            nn::Result
            CloseAll__Q2_2nn2acFv();

            // nn::Result
            // Connect__Q2_2nn2acFPC16netconf_profile_(const Config* cfg);

            nn::Result
            ReadConfig__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumP16netconf_profile_(ConfigIdNum id,
                                                                            Config* cfg);

            nn::Result
            IsConfigExisting__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumPb(ConfigIdNum id,
                                                                 bool* existing);

            nn::Result
            SetStartupId__Q2_2nn2acFQ3_2nn2ac11ConfigIdNum(ConfigIdNum id);

        }

    } // namespace detail

    nn::Result
    CloseAll()
    {
        return detail::CloseAll__Q2_2nn2acFv();
    }

    // nn::Result
    // Connect(const Config* cfg)
    // {
    //     return detail::Connect__Q2_2nn2acFPC16netconf_profile_(cfg);
    // }

    nn::Result
    ReadConfig(ConfigIdNum id, Config* cfg)
    {
        return detail::ReadConfig__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumP16netconf_profile_(id, cfg);
    }

    nn::Result
    IsConfigExisting(ConfigIdNum id, bool* existing)
    {
        return detail::IsConfigExisting__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumPb(id, existing);
    }

    nn::Result
    SetStartupId(ConfigIdNum id)
    {
        return detail::SetStartupId__Q2_2nn2acFQ3_2nn2ac11ConfigIdNum(id);
    }

} // namespace nn::ac


std::string
get_ssid(const NetConfWifiConfigData& cfg)
{
    std::size_t len = std::min<std::size_t>(cfg.ssidlength, sizeof cfg.ssid);
    return std::string(reinterpret_cast<const char*>(cfg.ssid),
                       len);
}


struct net_profile_item : wups::config::button_item {

    nn::ac::ConfigIdNum id;
    std::string description;

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
        try {
            nn_ac_guard guard;

            nn::ac::CloseAll();

            if (!nn::ac::Connect(id))
                throw runtime_error{"Failed to switch to profile " + std::to_string(id)};

            notify::infof("Using profile %u: %s",
                          id,
                          description.c_str());
        }
        catch (std::exception& e) {
            notify::errorf("Error: %s", e.what());
            logger::printf("Error: %s\n", e.what());
            status_msg = e.what();
        }
        current_state = state::stopped;
    }

};


void
menu_open(wups::config::category& root)
{
    logger::initialize(PACKAGE_NAME);
    notify::initialize(PACKAGE_NAME);

    nn_ac_guard guard;

    for (nn::ac::ConfigIdNum id = 1; id <= 6; ++id)
        root.add(net_profile_item::create(id));

    nn::ac::ConfigIdNum sid = 0;
    if (nn::ac::GetStartupId(&sid))
        startup_id = sid;

    root.add(wups::config::int_item::create("Default profile",
                                            startup_id,
                                            startup_id,
                                            1, 6));
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
