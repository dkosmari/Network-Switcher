/*
 * Network Switcher - A plugin to switch network profiles on the Wii U.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wut_extras.hpp"


extern "C" {

    nn::Result
    CloseAll__Q2_2nn2acFv();

    nn::Result
    Connect__Q2_2nn2acFPC16netconf_profile_(const nn::ac::Config* cfg);

    nn::Result
    ReadConfig__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumP16netconf_profile_(nn::ac::ConfigIdNum id,
                                                                    nn::ac::Config* cfg);

    nn::Result
    IsConfigExisting__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumPb(nn::ac::ConfigIdNum id,
                                                         bool* existing);

    nn::Result
    SetStartupId__Q2_2nn2acFQ3_2nn2ac11ConfigIdNum(nn::ac::ConfigIdNum id);


    nn::Result
    GetCompatId__Q2_2nn2acFPQ3_2nn2ac11ConfigIdNum(nn::ac::ConfigIdNum* id);

    nn::Result
    SetCompatId__Q2_2nn2acFQ3_2nn2ac11ConfigIdNum(nn::ac::ConfigIdNum id);

}


namespace nn::ac {

    nn::Result
    CloseAll()
    {
        return CloseAll__Q2_2nn2acFv();
    }


    nn::Result
    Connect(const Config* cfg)
    {
        return Connect__Q2_2nn2acFPC16netconf_profile_(cfg);
    }


    nn::Result
    ReadConfig(ConfigIdNum id, Config* cfg)
    {
        return ReadConfig__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumP16netconf_profile_(id, cfg);
    }


    nn::Result
    IsConfigExisting(ConfigIdNum id, bool* existing)
    {
        return IsConfigExisting__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumPb(id, existing);
    }


    nn::Result
    SetStartupId(ConfigIdNum id)
    {
        return SetStartupId__Q2_2nn2acFQ3_2nn2ac11ConfigIdNum(id);
    }


    nn::Result
    GetCompatId(ConfigIdNum* id)
    {
        return GetCompatId__Q2_2nn2acFPQ3_2nn2ac11ConfigIdNum(id);
    }


    nn::Result
    SetCompatId(ConfigIdNum id)
    {
        return SetCompatId__Q2_2nn2acFQ3_2nn2ac11ConfigIdNum(id);
    }

} // namespace nn::ac
