from bluepy.btle import Peripheral, UUID
from bluepy.btle import Scanner, DefaultDelegate

# Delegate class to handle notifications
class NotificationDelegate(DefaultDelegate):
    def __init__(self, params=None):
        DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
        # Handle the notification data from the peripheral
        print("Notification received from handle {}: {}".format(cHandle, data))

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev:
            print("Discovered device", dev.addr)
        elif isNewData:
            print("Received new data from", dev.addr)

# Start scanning for devices
scanner = Scanner().withDelegate(ScanDelegate())
devices = scanner.scan(10.0)

n = 0
addr = []
for dev in devices:
    print("%d: Device %s (%s), RSSI=%d dB" % (n, dev.addr, dev.addrType, dev.rssi))
    addr.append(dev.addr)
    n += 1
    for (adtype, desc, value) in dev.getScanData():
        print("%s = %s" % (desc, value))

# User selects a device to connect to
number = input('Enter your device number: ')
print('Device', number)
num = int(number)
print(addr[num])

# Connect to the selected device
print("Connecting...")
dev = Peripheral(addr[num], 'random')

# Assign the NotificationDelegate to handle notifications
dev.withDelegate(NotificationDelegate())

# List services of the connected device
print("Services...")
for svc in dev.services:
    print(str(svc))

try:
    # Get the characteristic with the specified UUID
    characteristics = dev.getCharacteristics(uuid=UUID(0x1111))[1]  # Update with your actual characteristic UUID
    print(characteristics)

    # Check if at least one characteristic was found
    if characteristics:
        # Find the CCCD descriptor
        cccd = None
        for descriptor in characteristics.getDescriptors():
            if descriptor.uuid == UUID(0x2902):  # Check for CCCD UUID
                cccd = descriptor
                break

        try:
            # Write to the CCCD to enable notifications (or indications, as per your use case)
            cccd.write(b'\x01\x00')  # Enable notifications (b'\x01\x00') or indications (b'\x02\x00')
            print("CCCD updated successfully to enable notifications.")

            # Read back the CCCD value
            value = cccd.read()
            print("CCCD current value:", value)

            # Verify if the value is correct
            if value == b'\x01\x00':  # Expecting notification enablement value
                print("Successfully confirmed that notifications are enabled.")
            else:
                print("Failed to confirm the CCCD update, current value:", value)

            # Now wait for notifications
            print("Waiting for notifications...")
            while True:
                if dev.waitForNotifications(1.0):  # This will wait for 1 second for a notification
                    # Notification handled in NotificationDelegate
                    continue

                print("Waiting...")

        except Exception as e:
            print("Failed to update CCCD:", e)
    else:
        print("No characteristic found with the specified UUID.")

finally:
    # Ensure the device is disconnected properly
    dev.disconnect()
    print("Disconnected.")
