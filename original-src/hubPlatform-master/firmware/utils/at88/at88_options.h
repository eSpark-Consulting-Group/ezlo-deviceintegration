#pragma once

#include "tiny/tiny_command_line_parser.h"

class At88Options {
public:
    At88Options();
    ~At88Options() = default;

public:
    void processCliOptions( int argc, char** argv );

private:
    void readEthernetMacAddress();
    void readWifiMacAddress();
    void readSerial();
    void readHubType();
    void readCpuSid();
    void readPkDevice();
    void readUuid();
    void readFirmwareVersion();
    void readUniqueHardwareJson();
    void printOut( const std::string& prefix, const std::string& value );
    void printOutJson( const std::string& prefix, const std::string& value );

private:
    tiny::command_line::parser m_parser;
    bool m_jsonFormattedOutput;
    bool m_appendPrefix;
};
