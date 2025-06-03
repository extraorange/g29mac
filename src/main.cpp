#include <cstdint>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>

const uint8_t G29_SetRange900[]        = { 0xf8, 0x81, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00 };
const uint8_t G29_SetRange540[]        = { 0xf8, 0x81, 0x1C, 0x02, 0x00, 0x00, 0x00, 0x00 };
const uint8_t G29_SetRange270[]        = { 0xf8, 0x81, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00 };
const uint8_t G29_EnableNativeMode[]   = { 0xf8, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t G29_Reset[]              = { 0xf3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t G29_EnableFFB[]          = { 0xf5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t G29_DisableFFB[]         = { 0xf6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Available, but redundant commands:
// const uint8_t G29_EnableClutch[]       = { 0xf8, 0x09, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00 };
// const uint8_t G29_StopAll[]            = { 0xf4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// const uint8_t G920_SetRangeAlt900[]     = { 0xf8, 0x61, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00 };
// const uint8_t G920_EnableNativeMode[] = { 0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

struct CmdArray {
    const char* name;
    const uint8_t* data;
    size_t lenght;
};

const CmdArray g29Cmd[] = {
    { "Set Range 900", G29_SetRange900, sizeof(G29_SetRange900) },
    { "Set Range 540", G29_SetRange540, sizeof(G29_SetRange540) },
    { "Set Range 270", G29_SetRange270, sizeof(G29_SetRange270) },
    { "Enable Native Mode", G29_EnableNativeMode, sizeof(G29_EnableNativeMode) },
    { "Reset Device", G29_Reset, sizeof(G29_Reset) },
    { "Enable Force Feedback", G29_EnableFFB, sizeof(G29_EnableFFB) },
    { "Disable Force Feedback", G29_DisableFFB, sizeof(G29_DisableFFB) }
};

void sendCommand(IOHIDDeviceRef device, const CmdArray& cmd) {
    IOReturn result = IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 0, cmd.data, cmd.lenght);

    if (result == kIOReturnSuccess) {
        std::cout << "[" << device <<"] Command sent: " << cmd.name << std::endl;
    } else {
        std::cout << "[" << device << "] Failed to send command: " << cmd.name << std::endl;
    }
    std::cout << std::endl;
};

void printDeviceProperties(IOHIDDeviceRef device) {
    auto printString = [](const char* label, CFStringRef str) {
        if (!str) return;
        
        char buffer[256];
        if (CFStringGetCString(str, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
            std::cout << "  " << label << ": " << buffer << std::endl;
        }
    };
    
    auto printNumber = [](const char* label, CFNumberRef num) {
        if (!num) return;
        
        int value = 0;
        if (CFNumberGetValue(num, kCFNumberIntType, &value)) {
            std::cout << "  " << label << ": " << value << std::endl;
        }
    };
    
    std::cout << "----------------- Device Info -----------------" << std::endl;
    printString("Product Name", (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey)));
    printNumber("Vendor ID", (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey)));
    printNumber("Product ID", (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey)));
    printString("Serial Number", (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDSerialNumberKey)));
    printNumber("Usage Page", (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey)));
    printNumber("Usage", (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsageKey)));
    printNumber("Location ID", (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDLocationIDKey)));
    printString("Transport", (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDTransportKey)));
    std::cout << "----------------- *********** -----------------" << std::endl;
};

bool isG29(IOHIDDeviceRef device) {
    CFTypeRef vendorIDRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
    CFTypeRef productIDRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
    CFTypeRef usagePageRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey));
    CFTypeRef usageRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsageKey));

    int vendorId = 0, productId = 0, usagePage = 0, usage = 0;
    if (vendorIDRef) CFNumberGetValue((CFNumberRef)vendorIDRef, kCFNumberIntType, &vendorId);
    if (productIDRef) CFNumberGetValue((CFNumberRef)productIDRef, kCFNumberIntType, &productId);
    if (usagePageRef) CFNumberGetValue((CFNumberRef)usagePageRef, kCFNumberIntType, &usagePage);
    if (usageRef) CFNumberGetValue((CFNumberRef)usageRef, kCFNumberIntType, &usage);

    return (vendorId == 0x046d && productId == 0xc24f && usagePage == 1 && usage == 4);
}

int main(int argc, char* argv[]) {
    std::cout << "------------------- G29&MAC -------------------" << std::endl;

    int mode = 0;
    if (argc >= 2) mode = std::atoi(argv[1]);

    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    IOHIDManagerSetDeviceMatching(manager, NULL);
    IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    CFSetRef deviceSet = IOHIDManagerCopyDevices(manager);

    if (deviceSet) {
        CFIndex count = CFSetGetCount(deviceSet);
        std::vector<IOHIDDeviceRef> devices(count);
        CFSetGetValues(deviceSet, (const void **)devices.data());

        // DEBUG
        // int i = 0;
        // for (IOHIDDeviceRef device : devices) {
        //     i++;
        //     std::cout << "Device #" << i << " : "<< device << std::endl;
        // }
        // std::cout << std::endl;

        bool success;
        for (IOHIDDeviceRef device : devices) {
            if (isG29(device)) {
                std::cout << "[" << device << "] " << "Initialised Logitech G29" << std::endl;
                printDeviceProperties(device);
                sendCommand(device, g29Cmd[mode]);
                success = true;
            };
        };
        
        if (!success) {
            std::cerr << "No Logitech G29 device found." << std::endl;
        }
    };

    return 0;
}