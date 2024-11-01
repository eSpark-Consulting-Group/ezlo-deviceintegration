#pragma once

#if defined( __HUB_TYPE_A20__ )

#include "spi/spidev.h"
#include "base_utils/spi/spi_transport.h"
static constexpr char spiDeviceFilename[] = "/dev/spidev2.0";
static constexpr char resetDeviceFilename[] = "/sys/class/gpio/gpio3_ph20/value";
class SpiTransport;
using EnvmTransport = SpiTransport;
using ZwaveTransport = SpiTransport;

#elif defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_BEEWAVE__ )

#include "spi/spidev_h2d1.h"
#include "base_utils/spi/spi_transport.h"
static constexpr char spiDeviceFilename[] = "/dev/spidev0.0";
#if defined( __HUB_TYPE_BEEWAVE__ )
static constexpr char resetDeviceFilename[] = "/sys/class/gpio/gpio15/value";
#else
static constexpr char resetDeviceFilename[] = "/sys/class/gpio/gpio106/value";
#endif
class SpiTransport;
using EnvmTransport = SpiTransport;
using ZwaveTransport = SpiTransport;

#elif defined( __HUB_TYPE_G150__ )

#include "spi/spidev.h"
#include "base_utils/spi/spi_soft_sercomm.h"
static constexpr char spiDeviceFilename[] = "/dev/null";
static constexpr char resetDeviceFilename[] = "/dev/null";
class SpiSoftSercomm;
using EnvmTransport = SpiSoftSercomm;
using ZwaveTransport = SpiSoftSercomm;

#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ ) || defined( __HUB_TYPE_G450__ ) || \
    defined( __HUB_TYPE_G550__ )

#include "spi/spidev.h"
#include "base_utils/spi/spi_transport.h"
static constexpr char spiDeviceFilename[] = "/dev/null";
static constexpr char resetDeviceFilename[] = "/dev/null";
class EnvmMockTransport;
using EnvmTransport = EnvmMockTransport;
using ZwaveTransport = EnvmMockTransport;

#else

#error "Specify valid HUB_TYPE for build..."

#endif
