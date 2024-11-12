from bluepy.btle import Peripheral, UUID, Scanner, DefaultDelegate, BTLEException

# Define the UUIDs for the GATT characteristics
ACCEL_DATA_UUID = UUID("00000000-0001-11e1-9ab4-0002a5d5c51b")  # Replace with actual UUID for acceleration data
WRITE_CHAR_UUID = UUID("00000000000111e1ac360002a5d5c51b")  # UUID of the characteristic to write

# Delegate class to handle notifications
class NotificationDelegate(DefaultDelegate):
    def __init__(self, dev):
        DefaultDelegate.__init__(self)
        self.dev = dev
        self.received_data_count = 0  # Counter to track the number of received notifications

    def handleNotification(self, cHandle, data):
        # Handle the notification data from the peripheral
        print("Notification received from handle {}: {}".format(cHandle, data))
        self.received_data_count += 1
        print(self.received_data_count)

        # If 10 notifications have been received, write a value to the characteristic
        if self.received_data_count >= 10:
            self.write_value_to_characteristic()

    def write_value_to_characteristic(self):
        try:
            # Prompt the user for the value to write
            value_to_write = int(input("Enter the value to write to the characteristic: "))
            
            # Get the characteristic with the specified UUID for writing
            write_char = self.dev.getCharacteristics(uuid=WRITE_CHAR_UUID)[0]
            if write_char:
                # Convert the integer to 8 bytes
                write_bytes = value_to_write.to_bytes(1, byteorder='little')  # Convert integer to 8 bytes
                write_char.write(write_bytes)
                print(f"Successfully wrote value {value_to_write} to characteristic {WRITE_CHAR_UUID}.")
                
                # Reset the counter after writing
                self.received_data_count = 0
        except Exception as e:
            print("Failed to write value to characteristic:", e)

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

# Display discovered devices
n = 0
addr = []
for dev in devices:
    print(f"{n}: Device {dev.addr} ({dev.addrType}), RSSI={dev.rssi} dB")
    addr.append(dev.addr)
    n += 1
    for (adtype, desc, value) in dev.getScanData():
        print(f"{desc} = {value}")

# User selects a device to connect to
try:
    number = input('Enter your device number: ')
    num = int(number)
    print(f'Connecting to device: {addr[num]}')

    # Connect to the selected device with 'random' address type
    dev = Peripheral(addr[num], 'random')
    print("Connected successfully.")

    # Assign the NotificationDelegate to handle notifications
    dev.withDelegate(NotificationDelegate(dev))

    # List services of the connected device
    print("Services...")
    for svc in dev.services:
        print(str(svc))

    # Get the characteristic with the specified UUID for acceleration data
    accel_char = dev.getCharacteristics(uuid=UUID("00E00000000111e1ac360002a5d5c51b"))[0]
    print("Acceleration Data Characteristic:", accel_char, type(accel_char))

    # Check if the characteristic exists
    if accel_char:
        # Find the CCCD descriptor for notifications
        cccd = None
        for descriptor in accel_char.getDescriptors():
            if descriptor.uuid == UUID(0x2902):  # Check for CCCD UUID
                cccd = descriptor
                break

        try:
            # Write to the CCCD to enable notifications
            cccd.write(b'\x01\x00')  # Enable notifications
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
        print("No characteristic found with the specified UUID for acceleration data.")

except (ValueError, IndexError) as e:
    print("Invalid selection or error in input:", e)
except Exception as e:
    print("An error occurred while connecting:", e)
finally:
    # Ensure the device is disconnected properly
    try:
        dev.disconnect()
        print("Disconnected.")
    except:
        print("Failed to disconnect properly.") looks like in this program, it doesn't enter write_char = self.dev.getCharacteristics(uuid=WRITE_CHAR_UUID)[0]
            if write_char:
                # Convert the integer to 8 bytes
                write_bytes = value_to_write.to_bytes(1, byteorder='little')  # Convert integer to 8 bytes
                write_char.write(write_bytes)
                print(f"Successfully wrote value {value_to_write} to characteristic {WRITE_CHAR_UUID}.")
                
                # Reset the counter after writing
                self.received_data_count = 0 are you sure the writing process in intact?