#include "hardware_manager/led/led_color_pwm.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"

namespace {
static constexpr uint8_t I2cChipAddress = 0x45;

static constexpr uint8_t SoftResetRegister = 0x00;
static constexpr uint8_t SoftResetValue = 0x55;

static constexpr uint8_t GlobalControlRegister = 0x01;
static constexpr uint8_t GCRLedFunctionEnableMask = 0x01;
static constexpr uint8_t GCRLedFunctionDisableMask = 0x00;

static constexpr uint8_t LEDControlRegister = 0x30;

// static constexpr uint8_t GreenLedEnableMask = ( 1 << 0 );
// static constexpr uint8_t RedLedEnableMask = ( 1 << 1 );
// static constexpr uint8_t BlueLedEnableMask = ( 1 << 2 );
static constexpr uint8_t AllLedEnableMask = 0x07;
static constexpr uint8_t AllLedDisableMask = 0x00;

// TODO: mode control registers are not valid for beewave, however it's not a problem at this moment
static constexpr uint8_t GreenLEDModeControlRegister = 0x31;
static constexpr uint8_t RedLEDModeControlRegister = 0x32;
static constexpr uint8_t BlueLEDModeControlRegister = 0x33;
static constexpr uint8_t LedMaxCurrent_0ma = 0;
static constexpr uint8_t LedMaxCurrent_5ma = 1;
static constexpr uint8_t LedMaxCurrent_10ma = 2;
static constexpr uint8_t LedMaxCurrent_15ma = 3;

}  // namespace

LedColorPwm::LedColorPwm( std::string name )
: LedColor( std::move( name ) )
, m_i2cControl( nullptr )
, m_redAddr( 0 )
, m_greenAddr( 0 )
, m_blueAddr( 0 ) {}

LedColorPwm::LedColorPwm( std::string name,
                          const std::string& devPath,
                          i2c::Byte redAddr,
                          i2c::Byte greenAddr,
                          i2c::Byte blueAddr )
: LedColor( std::move( name ) )
, m_i2cControl( std::unique_ptr< i2c::I2cControl >( new i2c::I2cControl( devPath ) ) )
, m_redAddr( redAddr )
, m_greenAddr( greenAddr )
, m_blueAddr( blueAddr ) {
    initBus();
}

LedColorPwm::~LedColorPwm() {
    m_i2cControl.reset();
}

void LedColorPwm::initBus() {
    m_i2cControl->write( I2cChipAddress, { SoftResetRegister, SoftResetValue } );
    m_i2cControl->write( I2cChipAddress, { GlobalControlRegister, GCRLedFunctionEnableMask } );
    m_i2cControl->write( I2cChipAddress, { GreenLEDModeControlRegister, LedMaxCurrent_5ma } );
    m_i2cControl->write( I2cChipAddress, { RedLEDModeControlRegister, LedMaxCurrent_5ma } );
    m_i2cControl->write( I2cChipAddress, { BlueLEDModeControlRegister, LedMaxCurrent_5ma } );
    turnOff();
}

bool LedColorPwm::turnOn() {
    if( state == LedState::LedBlinking ) {
        m_i2cControl->write( I2cChipAddress, { GreenLEDModeControlRegister, LedMaxCurrent_5ma } );
        m_i2cControl->write( I2cChipAddress, { RedLEDModeControlRegister, LedMaxCurrent_5ma } );
        m_i2cControl->write( I2cChipAddress, { BlueLEDModeControlRegister, LedMaxCurrent_5ma } );
    }
    m_i2cControl->write( I2cChipAddress, { LEDControlRegister, AllLedEnableMask } );
    state = LedState::LedOn;
    return true;
}

bool LedColorPwm::turnOff() {
    if( state == LedState::LedBlinking ) {
        m_i2cControl->write( I2cChipAddress, { GreenLEDModeControlRegister, LedMaxCurrent_5ma } );
        m_i2cControl->write( I2cChipAddress, { RedLEDModeControlRegister, LedMaxCurrent_5ma } );
        m_i2cControl->write( I2cChipAddress, { BlueLEDModeControlRegister, LedMaxCurrent_5ma } );
    }
    m_i2cControl->write( I2cChipAddress, { LEDControlRegister, AllLedDisableMask } );
    state = LedState::LedOff;
    return true;
}

bool LedColorPwm::turn( bool enable ) {
    return ( enable ) ? turnOn() : turnOff();
}

bool LedColorPwm::turnBlinking( int msOff, int msOn ) {
    if( m_redAddr != 0x35 || m_greenAddr != 0x34 || m_blueAddr != 0x36 ||
        ( m_color.red && m_color.green && m_color.red != m_color.green ) ||
        ( m_color.red && m_color.blue && m_color.red != m_color.blue ) ||
        ( m_color.green && m_color.blue && m_color.green != m_color.blue ) ) {
        Log::warn( cc::warn( "LedColorPwm::turnBlinking not implemented yet." ) );
        return false;
    }

    uint8_t t1t2 = log2( msOn / 125 );
    uint8_t t3t4 = log2( msOff / 125 );
    if( t1t2 > 5 )
        t1t2 = 5;
    if( t3t4 > 7 )
        t3t4 = 7;

    m_i2cControl->write( I2cChipAddress, { 0x30, 0x00 } );
    m_i2cControl->write( I2cChipAddress, { 0x31, 0x01 } );
    m_i2cControl->write( I2cChipAddress, { 0x32, 0x01 } );
    m_i2cControl->write( I2cChipAddress, { 0x33, 0x01 } );
    unsigned char enable = 0;
    if( m_color.red ) {
        m_i2cControl->write( I2cChipAddress, { 0x3a, t1t2 } );
        m_i2cControl->write( I2cChipAddress, { 0x3b, t3t4 } );
        m_i2cControl->write( I2cChipAddress, { 0x3c, 0x00 } );
        enable |= 0x02;
    }
    if( m_color.green ) {
        m_i2cControl->write( I2cChipAddress, { 0x37, t1t2 } );
        m_i2cControl->write( I2cChipAddress, { 0x38, t3t4 } );
        m_i2cControl->write( I2cChipAddress, { 0x39, 0x00 } );
        enable |= 0x01;
    }
    if( m_color.blue ) {
        m_i2cControl->write( I2cChipAddress, { 0x3d, t1t2 } );
        m_i2cControl->write( I2cChipAddress, { 0x3e, t3t4 } );
        m_i2cControl->write( I2cChipAddress, { 0x3f, 0x00 } );
        enable |= 0x04;
    }
    m_i2cControl->write( I2cChipAddress, { 0x31, 0x11 } );
    m_i2cControl->write( I2cChipAddress, { 0x32, 0x11 } );
    m_i2cControl->write( I2cChipAddress, { 0x33, 0x11 } );
    m_i2cControl->write( I2cChipAddress, { 0x30, enable } );
    state = LedState::LedBlinking;
    return true;
}

void LedColorPwm::setColor( Rgb color ) {
    m_color = color;
    m_i2cControl->write( I2cChipAddress, { m_redAddr, m_color.red } );
    m_i2cControl->write( I2cChipAddress, { m_greenAddr, m_color.green } );
    m_i2cControl->write( I2cChipAddress, { m_blueAddr, m_color.blue } );
}
