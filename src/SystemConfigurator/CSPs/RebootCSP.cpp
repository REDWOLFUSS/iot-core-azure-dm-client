#include "stdafx.h"
#include <map>
#include "RebootCSP.h"
#include "MdmProvision.h"
#include "..\SharedUtilities\DMException.h"
#include "..\SharedUtilities\Logger.h"
#include "..\SharedUtilities\JsonHelpers.h"
#include "..\SharedUtilities\TimeHelpers.h"

// Json strings
#define LastRebootTime L"lastRebootTime"
#define LastRebootCmdTime L"lastRebootCmdTime"
#define SingleRebootTime L"singleRebootTime"
#define DailyRebootTime L"dailyRebootTime"

using namespace Windows::System;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::System::Profile;
using namespace Windows::Foundation::Collections;

using namespace std;

#ifdef GetObject
#undef GetObject
#endif

// Reboot CSP docs
// https://msdn.microsoft.com/en-us/library/windows/hardware/mt720802(v=vs.85).aspx
//

const wchar_t* IoTDMRegistryRoot = L"Software\\Microsoft\\IoTDM";
const wchar_t* IoTDMRegistryLastRebootCmd = L"LastRebootCmd";

wstring RebootCSP::_lastRebootTime;

RebootCSP gRebootCSP;

RebootCSP::RebootCSP()
{
    TRACE(__FUNCTION__);

    _lastRebootTime = Utils::GetCurrentDateTimeString();
}

void RebootCSP::ExecRebootNow()
{
    TRACE(__FUNCTION__);

    Utils::WriteRegistryValue(IoTDMRegistryRoot, IoTDMRegistryLastRebootCmd, Utils::GetCurrentDateTimeString());

    TRACE(L"\n---- Run Reboot Now\n");
    MdmProvision::RunExec(L"./Device/Vendor/MSFT/Reboot/RebootNow");
}

void RebootCSP::SetRebootInfo(const std::wstring& jsonString)
{
    TRACE(__FUNCTION__);
    TRACEP(L" json = ", jsonString.c_str());
    /*
    {
        "rebootInfo":
        {
            "lastRebootTime" : ""
            "lastRebootCmdTime" : ""
            "singleRebootTime" : ""
            "dailyRebootTime" : ""
        }
    }
    */

    JsonValue^ value;
    if (!JsonValue::TryParse(ref new String(jsonString.c_str()), &value) || (value == nullptr))
    {
        throw DMException("Warning: SetRebootInfo(): Invalid json.");
    }

    JsonObject^ rootObject = value->GetObject();
    if (rootObject == nullptr)
    {
        throw DMException("Warning: SetRebootInfo(): Invalid json input. Cannot find the root object.");
    }

    map<wstring, IJsonValue^> properties;
    JsonReader::Flatten(L"", rootObject, properties);

    wstring singleRebootTime;
    if (JsonReader::TryFindString(properties, SingleRebootTime, singleRebootTime))
    {
        SetSingleScheduleTime(singleRebootTime);
    }

    wstring dailyRebootTime;
    if (JsonReader::TryFindString(properties, DailyRebootTime, dailyRebootTime))
    {
        SetDailyScheduleTime(dailyRebootTime);
    }

    TRACE(L"Reboot settings have been applied successfully.");
}

std::wstring RebootCSP::GetRebootInfoJson()
{
    TRACE(__FUNCTION__);

    JsonObject^ rebootInfoJson = ref new JsonObject();

    rebootInfoJson->Insert(ref new Platform::String(LastRebootTime),
                           JsonValue::CreateStringValue(ref new String(GetLastRebootTime().c_str())));

    rebootInfoJson->Insert(ref new Platform::String(LastRebootCmdTime),
                           JsonValue::CreateStringValue(ref new String(GetLastRebootCmdTime().c_str())));

    rebootInfoJson->Insert(ref new Platform::String(SingleRebootTime),
                           JsonValue::CreateStringValue(ref new String(GetSingleScheduleTime().c_str())));

    rebootInfoJson->Insert(ref new Platform::String(DailyRebootTime),
                           JsonValue::CreateStringValue(ref new String(GetDailyScheduleTime().c_str())));

    wstring json = rebootInfoJson->Stringify()->Data();

    TRACEP(L" json = ", json.c_str());

    return json;
}

wstring RebootCSP::GetLastRebootCmdTime()
{
    TRACE(__FUNCTION__);

    wstring lastRebootCmdTime = L"";
    try
    {
        Utils::ReadRegistryValue(IoTDMRegistryRoot, IoTDMRegistryLastRebootCmd);
    }
    catch (DMException&)
    {
        // This is okay to ignore since this device might have never received a reboot command through DM.
    }
    return lastRebootCmdTime;
}

std::wstring RebootCSP::GetLastRebootTime()
{
    TRACE(__FUNCTION__);

    return _lastRebootTime;
}

wstring RebootCSP::GetSingleScheduleTime()
{
    TRACE(L"\n---- Get Single Schedule Time\n");
    wstring time = MdmProvision::RunGetString(L"./Device/Vendor/MSFT/Reboot/Schedule/Single");
    time = Utils::CanonicalizeDateTime(time);   // CSP sometimes returns 2016-10-10T09:00:01-008:00, the 008 breaks .net parsing.
    TRACEP(L"    :", time.c_str());
    return time;
}

void RebootCSP::SetSingleScheduleTime(const wstring& dailyScheduleTime)
{
    TRACE(L"\n---- Set Single Schedule Time\n");
    MdmProvision::RunSet(L"./Device/Vendor/MSFT/Reboot/Schedule/Single", dailyScheduleTime);
    TRACEP(L"    :", dailyScheduleTime.c_str());
}

wstring RebootCSP::GetDailyScheduleTime()
{
    TRACE(L"\n---- Get Daily Schedule Time\n");
    wstring time = MdmProvision::RunGetString(L"./Device/Vendor/MSFT/Reboot/Schedule/DailyRecurrent");
    time = Utils::CanonicalizeDateTime(time);   // CSP sometimes returns 2016-10-10T09:00:01-008:00, the 008 breaks .net parsing.
    TRACEP(L"    :", time.c_str());
    return time;
}

void RebootCSP::SetDailyScheduleTime(const wstring& dailyScheduleTime)
{
    TRACE(L"\n---- Set Daily Schedule Time\n");
    MdmProvision::RunSet(L"./Device/Vendor/MSFT/Reboot/Schedule/DailyRecurrent", dailyScheduleTime);
    TRACEP(L"    :", dailyScheduleTime.c_str());
}