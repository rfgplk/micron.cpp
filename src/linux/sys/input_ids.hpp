
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

namespace micron
{

constexpr static const u64 id_bus = 0;
constexpr static const u64 id_vendor = 1;
constexpr static const u64 id_product = 2;
constexpr static const u64 id_version = 3;

constexpr static const u64 bus_pci = 0x01;
constexpr static const u64 bus_isapnp = 0x02;
constexpr static const u64 bus_usb = 0x03;
constexpr static const u64 bus_hil = 0x04;
constexpr static const u64 bus_bluetooth = 0x05;
constexpr static const u64 bus_virtual = 0x06;

constexpr static const u64 bus_isa = 0x10;
constexpr static const u64 bus_i8042 = 0x11;
constexpr static const u64 bus_xtkbd = 0x12;
constexpr static const u64 bus_rs232 = 0x13;
constexpr static const u64 bus_gameport = 0x14;
constexpr static const u64 bus_parport = 0x15;
constexpr static const u64 bus_amiga = 0x16;
constexpr static const u64 bus_adb = 0x17;
constexpr static const u64 bus_i2c = 0x18;
constexpr static const u64 bus_host = 0x19;
constexpr static const u64 bus_gsc = 0x1a;
constexpr static const u64 bus_atari = 0x1b;
constexpr static const u64 bus_spi = 0x1c;
constexpr static const u64 bus_rmi = 0x1d;
constexpr static const u64 bus_cec = 0x1e;
constexpr static const u64 bus_intel_ishtp = 0x1f;
constexpr static const u64 bus_amd_sfh = 0x20;

constexpr static const u64 mt_tool_finger = 0x00;
constexpr static const u64 mt_tool_pen = 0x01;
constexpr static const u64 mt_tool_palm = 0x02;
constexpr static const u64 mt_tool_dial = 0x0a;
constexpr static const u64 mt_tool_max = 0x0f;

constexpr static const u64 ff_status_stopped = 0x00;
constexpr static const u64 ff_status_playing = 0x01;
constexpr static const u64 ff_status_max = 0x01;

};
