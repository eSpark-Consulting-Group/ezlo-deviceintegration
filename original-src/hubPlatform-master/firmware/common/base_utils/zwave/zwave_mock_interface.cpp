#include "base_utils/zwave/zwave_mock_interface.h"

const std::string& ZwaveMockInterface::getName() const {
    static const std::string name = "ZwaveMock";
    return name;
}

bool ZwaveMockInterface::isSupported() {
    return true;
}

bool ZwaveMockInterface::readID( ChipMemoryVector& outResult ) {
    outResult.clear();
    return true;
}

bool ZwaveMockInterface::readFlashCrc( ChipMemoryVector& outResult ) {
    outResult.clear();
    return true;
}

bool ZwaveMockInterface::checkFlashCrc() {
    return true;
}

size_t ZwaveMockInterface::readFlash( ChipMemoryVector& outResult ) {
    outResult.clear();
    return outResult.size();
}

size_t ZwaveMockInterface::writeFlash( const ChipMemoryVector& data ) {
    return data.size();
}

size_t ZwaveMockInterface::readNvr( ChipMemoryVector& outResult ) {
    outResult.clear();
    return outResult.size();
}

size_t ZwaveMockInterface::writeNvr( const ChipMemoryVector& data ) {
    return data.size();
}

uint16_t ZwaveMockInterface::calcNvrCrc( const ChipMemoryVector& data ) {
    return 0;
}

const size_t ZwaveMockInterface::getNvrSize() const {
    return 0;
}

bool ZwaveMockInterface::bulkErase() {
    return true;
}

std::string ZwaveMockInterface::nvrGetRegion( const std::vector< uint8_t >& nvrData ) {
    return "";
}

void ZwaveMockInterface::nvrSetRegion( const std::string& newRegion, ChipMemoryVector& nvrData ) {}

uint32_t ZwaveMockInterface::nvrGetSawC( const ChipMemoryVector& nvrData ) {
    return 0;
}

void ZwaveMockInterface::nvrSetSawC( uint32_t nvrSawCValue, ChipMemoryVector& nvrData ) {}

bool ZwaveMockInterface::setProgrammingOn() {
    return true;
}

bool ZwaveMockInterface::setProgrammingOff() {
    return true;
}

int32_t ZwaveMockInterface::init( uint32_t speed, const bool keepCSActivated ) {
    return 0;
}

void ZwaveMockInterface::deinit() {}

int ZwaveMockInterface::getDescriptor() const {
    return 0;
}

int ZwaveMockInterface::sendRequest( const std::vector< uint8_t >& command,
                                     int32_t resultSize,
                                     std::vector< uint8_t >& outResult ) {
    return resultSize;
}

bool ZwaveMockInterface::isValid() const {
    return true;
}
