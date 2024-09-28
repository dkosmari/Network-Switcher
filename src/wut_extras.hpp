/*
 * Network Switcher - A plugin to switch network profiles on the Wii U.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WUT_EXTRAS_HPP
#define WUT_EXTRAS_HPP

#include <nn/ac.h>
#include <nsysnet/netconfig.h>


namespace nn::ac {

    using Config = ::NetConfCfg;

    nn::Result CloseAll();

    nn::Result Connect(const Config* cfg);

    nn::Result ReadConfig(ConfigIdNum id, Config* cfg);

    nn::Result IsConfigExisting(ConfigIdNum id, bool* existing);

    nn::Result SetStartupId(ConfigIdNum id);

    nn::Result GetCompatId(ConfigIdNum* id);
    nn::Result SetCompatId(ConfigIdNum id);

} // namespace nn::ac


#endif
