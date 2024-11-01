#pragma once

#include <cstdint>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

namespace ConfigurationClass {

namespace SafetySubclass {
constexpr uint8_t ID = 2;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { OverTemp = 0, UnderTemp = 2, TempHys = 4 };
}  // namespace SafetySubclass

namespace ChargeTerminationSubclass {
constexpr uint8_t ID = 36;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { TCASet = 3, TCAClear = 4, FCSet = 5, FCClear = 6, DODatEOCDeltaT = 7 };
}  // namespace ChargeTerminationSubclass

namespace DataSubclass {
constexpr uint8_t ID = 48;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { InitialStandby = 2, InitialMaxLoad = 3 };
}  // namespace DataSubclass

namespace DischargeSubclass {
constexpr uint8_t ID = 49;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { SOC1SetThreshold = 0, SOC1ClearThreshold = 1, SOCFSetThreshold = 2, SOCFClearThreshold = 3 };
}  // namespace DischargeSubclass

namespace RegistersSubclass {
constexpr uint8_t ID = 64;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { OpConfig = 0, OpConfigB = 2 };
}  // namespace RegistersSubclass

namespace PowerSubclass {
constexpr uint8_t ID = 68;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { HibernateI = 7, HibernateV = 9 };
}  // namespace PowerSubclass

}  // namespace ConfigurationClass
namespace GasGaugingClass {

namespace ITCfgSubclassBlock0 {
constexpr uint8_t ID = 80;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t {
    RaFilter = 22,
};
}  // namespace ITCfgSubclassBlock0

namespace ITCfgSubclassBlock1 {
constexpr uint8_t ID = 80;
constexpr uint8_t BlockIdx = 0x01;
enum Offsets : uint8_t {
    FastQmaxStartDOD = 35,
    FastQmaxEndDOD = 36,
    FastQmaxStartVoltDelta = 37,
    FastQmaxCurrentThreshold = 39,
    FastQmaxMinPoints = 41,
    MaxQmaxChange = 45,
    QmaxMaxDelta = 46,
    MaxDefaultQmax = 47,
    QmaxFilter = 48,
    ResRelaxTime = 50,
    UserRatemA = 52,
    UserRatemW = 54,
    MaxSimRate = 61,
    MinSimRate = 62,
    //    RaMaxDelta = 63 // NOTE: no idea how to properly read this value
};
}  // namespace ITCfgSubclassBlock1

namespace ITCfgSubclassBlock2 {
constexpr uint8_t ID = 80;
constexpr uint8_t BlockIdx = 0x02;
enum Offsets : uint8_t { MinDeltaVoltage = 72, MaxDeltaVoltage = 74, DeltaVMaxdV = 76, TermVValidt = 78 };
}  // namespace ITCfgSubclassBlock2

namespace CurrentThresholdsSubclass {
constexpr uint8_t ID = 81;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t {
    DsgCurrentThreshold = 0,
    ChgCurrentThreshold = 2,
    QuitCurrent = 4,
    DsgRelaxTime = 6,
    ChgRelaxTime = 8,
    QuitRelaxTime = 9,
    MaxIRCorrect = 12
};
}  // namespace CurrentThresholdsSubclass

namespace StateSubclassBlock0 {
constexpr uint8_t ID = 82;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t {
    QmaxCell0 = 0,
    UpdateStatus = 2,
    ReserveCapmAh = 3,
    LoadSelectMode = 5,
    QInvalidMaxV = 6,
    QInvalidMinV = 8,
    DesignCapacity = 10,
    DesignEnergy = 12,
    DefaultDesignCap = 14,
    TerminateVoltage = 16,
    TRise = 22,
    TTimeConstant = 24,
    SOCIDelta = 26,
    TaperRate = 27,
    TaperVoltage = 29,
    //    SleepCurrent = 31 // NOTE: no idea how to properly read this value
};

}  // namespace StateSubclassBlock0

namespace StateSubclassBlock1 {
constexpr uint8_t ID = 82;
constexpr uint8_t BlockIdx = 0x01;
enum Offsets : uint8_t { VatChgTerm = 33, AvgILastRun = 35, AvgPLastRun = 37, DeltaVoltage = 39 };
}  // namespace StateSubclassBlock1

}  // namespace GasGaugingClass
namespace RaTablesClass {

namespace R_aRAMSubclass {
constexpr uint8_t ID = 89;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t {
    R_a0_0 = 0,
    R_a0_1 = 2,
    R_a0_2 = 4,
    R_a0_3 = 6,
    R_a0_4 = 8,
    R_a0_5 = 10,
    R_a0_6 = 12,
    R_a0_7 = 14,
    R_a0_8 = 16,
    R_a0_9 = 18,
    R_a0_10 = 20,
    R_a0_11 = 22,
    R_a0_12 = 24,
    R_a0_13 = 26,
    R_a0_14 = 28,
};
}  // namespace R_aRAMSubclass

}  // namespace RaTablesClass
namespace CalibrationClass {

namespace DataSubclass {
constexpr uint8_t ID = 104;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { BoardOffset = 0, IntTempOffset = 1, PackVOffset = 2 };
}  // namespace DataSubclass

namespace CCCalSubclass {
constexpr uint8_t ID = 105;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { CCOffset = 0, CCCalTemp = 2, CCGain = 4, CCDelta = 8 };
}  // namespace CCCalSubclass

namespace CurrentSubclass {
constexpr uint8_t ID = 107;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { Deadband = 1 };
}  // namespace CurrentSubclass

}  // namespace CalibrationClass
namespace SecurityClass {

namespace CodesSubclass {
constexpr uint8_t ID = 112;
constexpr uint8_t BlockIdx = 0x00;
enum Offsets : uint8_t { SealedtoUnsealed = 0 };
}  // namespace CodesSubclass

}  // namespace SecurityClass

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
