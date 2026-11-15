#include "api.hpp"
#include "ByteArray.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include <mtpz/TrustedApp.h>
#include <ptp/Device.h>
#include <ptp/ByteArrayObjectStream.h>
#include <ptp/PipePacketer.h>
#include <usb/Context.h>
#include <usb/Device.h>
#include <usb/DeviceBusyException.h>
#include <usb/DeviceDescriptor.h>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::string GetMtpzDataPath() {
    char* home = getenv("HOME");
    fs::path mtpz(".mtpz-data");

    if(home == nullptr) {
        return mtpz;
    }

    fs::path home_variant = fs::path(home) / mtpz;
    if(fs::exists(home_variant)) {
        return home_variant;
    }

    return mtpz;
}

ZuneDevice::ZuneDevice(const mtp::ByteArray& identification, mtp::DevicePtr& device, mtp::SessionPtr& session, mtp::TrustedAppPtr& ta)
    : device(device), session(session), ta(ta), identification(identification) {}

ZuneDevice::ZuneDevice(const mtp::ByteArray& identification, mtp::DevicePtr& device, mtp::SessionPtr& session)
    : device(device), session(session), ta(nullptr), identification(identification) {}

ZuneDevice::~ZuneDevice() {}

auto OpenConnection(ZuneDevice::Ptr* out_devicePtr) -> Result {
    *out_devicePtr = 0;

    try {
        auto device = mtp::Device::FindFirst();
        if (!device) {
            return Result::ErrorNoDevice;
        }

        auto session = device->OpenSession(1);
        auto devinfo = session->GetDeviceInfo();

        mtp::ByteArray identificaiton = session->GetDeviceProperty(mtp::DeviceProperty(0xD21A));

        // Expected to fail when the session has not been authenticated yet
        bool sessionAuthenticated = true;
        try {
            session->XnaOpenSession();
        } catch (...) {
            sessionAuthenticated = false;
        }

        if (!sessionAuthenticated) {
            auto ta = mtp::TrustedApp::Create(session, GetMtpzDataPath());

            if (!ta) {
                return Result::ErrorHandshakeFailed;
            }

            ta->Authenticate(true);

            *out_devicePtr = new ZuneDevice(identificaiton, device, session, ta);
        } else {
            *out_devicePtr = new ZuneDevice(identificaiton, device, session);
        }
    } catch (...) {
        return Result::ErrorConnectionFailed;
    }

    return Result::Ok;
}

auto CloseConnection(ZuneDevice::Ptr device) -> void {
    try {
        device->session->XnaCloseSession();
        delete device;
    } catch (...) { }
}

auto PollData(ZuneDevice::Ptr device, std::uint8_t* out_buffer, std::size_t size, std::size_t* out_bytesRead)
    -> Result {
    try {
        auto result = device->session->XnaPollData();
        *out_bytesRead = result.size();

        if (result.empty()) {
            return Result::Ok;
        }

        if (size < result.size()) {
            return Result::ErrorBufferTooSmall;
        }
        std::memcpy(out_buffer, result.data(), std::min(size, result.size()));
    } catch (...) {
        return Result::ErrorReadFailed;
    }

    return Result::Ok;
}

auto SendData(ZuneDevice::Ptr device, std::uint8_t* buffer, std::size_t size) -> Result {
    try {
        std::vector<std::uint8_t> data(buffer, buffer + size);
        auto response = device->session->XnaSendData(data);
    } catch (...) {
        return Result::ErrorSendFailed;
    }

    return Result::Ok;
}

auto GetDeviceFamily(ZuneDevice::Ptr device) -> uint8_t {
    return device->identification[3];
}

auto OpenTransport(std::uint16_t vendorId, std::uint16_t productId, ZuneTransport::Ptr* out_transport) -> Result {
    *out_transport = nullptr;
    bool foundButNotOpened = false;

    try {
        auto context = std::make_shared<mtp::usb::Context>();
        for (mtp::usb::DeviceDescriptorPtr desc : context->GetDevices()) {
            if (desc->GetVendorId() != vendorId || desc->GetProductId() != productId) {
                continue;
            }

            auto device = desc->TryOpen(context);
            if (!device) {
                foundButNotOpened = true;
                continue;
            }

            auto conf = desc->GetConfiguration(0);
            if (conf->GetInterfaceCount() == 0) {
                // Will it work in mac?
                int index = conf->GetIndex();
                conf.reset();
                device->SetConfiguration(index);
                conf = desc->GetConfiguration(0);
            }

            // We have only a single one
            auto iface = conf->GetInterface(device, conf, 0, 0);
            auto token = device->ClaimInterface(iface);

            *out_transport = new ZuneTransport{mtp::usb::BulkPipe::Create(device, conf, iface, token)};
            return Result::Ok;
        }
    } catch (const mtp::usb::DeviceBusyException&) {
        return Result::ErrorDeviceBusy;
    } catch (...) {
        return Result::ErrorConnectionFailed;
    }

    return foundButNotOpened ? Result::ErrorAccessDenied : Result::ErrorNoDevice;
}

auto CloseTransport(ZuneTransport::Ptr transport) -> void {
    delete transport;
}

auto TransportWrite(ZuneTransport::Ptr transport, const std::uint8_t* buffer, std::size_t size, int timeoutMs) -> Result {
    try {
        transport->pipe->Write(std::make_shared<mtp::ByteArrayObjectInputStream>(mtp::ByteArray(buffer, buffer + size)), timeoutMs);
    } catch (...) {
        return Result::ErrorSendFailed;
    }

    return Result::Ok;
}

auto TransportRead(ZuneTransport::Ptr transport, std::uint8_t* out_buffer, std::size_t size, std::size_t* out_bytesRead, int timeoutMs) -> Result {
    try {
        auto stream = std::make_shared<mtp::ByteArrayObjectOutputStream>();
        transport->pipe->Read(stream, timeoutMs);
        auto& data = stream->GetData();
        if (data.size() > size) {
            return Result::ErrorBufferTooSmall;
        }

        std::memcpy(out_buffer, data.data(), data.size());
        *out_bytesRead = data.size();
    } catch (...) {
        return Result::ErrorReadFailed;
    }

    return Result::Ok;
}
