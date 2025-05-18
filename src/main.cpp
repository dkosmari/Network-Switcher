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
#include <wupsxx/notify.hpp>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using std::runtime_error;

namespace logger = wups::logger;
namespace notify = wups::notify;


WUPS_PLUGIN_NAME(PACKAGE_NAME);
WUPS_PLUGIN_VERSION(PACKAGE_VERSION);
WUPS_PLUGIN_DESCRIPTION("Switch between network profiles.");
WUPS_PLUGIN_AUTHOR("Daniel K. O.");
WUPS_PLUGIN_LICENSE("GPLv3");


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


struct net_profile_item : wups::button_item {

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

            if (cfg.wl0.if_state)
                description = "[Wi-Fi] SSID=\"" + get_ssid(cfg.wifi.config) + "\"";
            else if (cfg.eth0.if_state)
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
            notify::info::show("Using profile %u: %s",
                               static_cast<unsigned>(id),
                               description.c_str());
        }
        catch (std::exception& e) {
            notify::error::show("Error: %s", e.what());
            logger::printf("Error: %s\n", e.what());
            status_msg = e.what();
        }
    }

};


struct disconnect_item : wups::button_item {

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


namespace cfg {

    WUPSXX_OPTION("Default profile",
                  int, startup_id, 1, 1, 6);

    WUPSXX_OPTION("vWii profile",
                  int, compat_id, 1, 1, 6);

} // namespace cfg


void
menu_open(wups::category& root)
{
    using wups::make_item;

    logger::initialize();

    nn_ac_guard guard;

    for (nn::ac::ConfigIdNum id = 1; id <= 6; ++id)
        root.add(net_profile_item::create(id));

    nn::ac::ConfigIdNum sid = 0;
    if (nn::ac::GetStartupId(&sid))
        cfg::startup_id.value = sid;
    root.add(make_item(cfg::startup_id));

    nn::ac::ConfigIdNum cid = 0;
    if (nn::ac::GetCompatId(&cid))
        cfg::compat_id.value = cid;
    root.add(make_item(cfg::compat_id));

    root.add(disconnect_item::create());
}


void
menu_close()
{
    nn_ac_guard guard;

    nn::ac::ConfigIdNum sid = 0;
    if (nn::ac::GetStartupId(&sid) && static_cast<int>(sid)
        != cfg::startup_id.value) {
        if (nn::ac::SetStartupId(cfg::startup_id.value)) {
            notify::info::show("Set default profile to %d",
                               cfg::startup_id.value);
        } else {
            notify::error::show("Could not set default profile to %d",
                                cfg::startup_id.value);
            logger::printf("nn::ac::SetStartupId(%d) failed\n",
                           cfg::startup_id.value);
        }
    }

    nn::ac::ConfigIdNum cid = 0;
    if (nn::ac::GetCompatId(&cid) && static_cast<int>(cid)
        != cfg::compat_id.value) {
        if (nn::ac::SetCompatId(cfg::compat_id.value)) {
            notify::info::show("Set vWii profile to %d",
                               cfg::compat_id.value);
        } else {
            notify::error::show("Could not set vWii profile to %d",
                                cfg::compat_id.value);
            logger::printf("nn::ac::SetCompatId(%d) failed\n",
                           cfg::compat_id.value);
        }
    }

    logger::finalize();
}


INITIALIZE_PLUGIN()
{
    logger::set_prefix(PACKAGE_NAME);
    logger::guard guard;
    notify::initialize(PACKAGE_NAME);

    try {
        wups::init(PACKAGE_NAME, menu_open, menu_close);
    }
    catch (std::exception& e) {
        logger::printf("ERROR: %s\n", e.what());
    }
}


DEINITIALIZE_PLUGIN()
{
    notify::finalize();
}
