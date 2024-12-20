from bluepy.btle import Scanner, Peripheral, DefaultDelegate, UUID, BTLEException
import time
import struct

svc_sample = 0
char_sample = 0

# Delegate to handle notifications
class MyDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    # This method is called when a notification is received
    def handleNotification(self, cHandle, data):
        if len(data) == 6:  # Ensure the data length is 6 bytes (3 * 2-byte integers)
            # Unpack the 6 bytes as 3 little-endian signed 16-bit integers
            x, y, z = struct.unpack('<hhh', data)
            
            # Print the coordinates in x, y, z format
            print(f"x: {x}, y: {y}, z: {z}")
        else:
            # If the data length is not 6 bytes, print the raw data
            print(f"Notification received from handle: {cHandle}, data: {data.hex()}")

def connect_device(target_name):
    scanner = Scanner()
    devices = scanner.scan(5.0)  # Scan for 5 seconds
    
    # Find device by name
    target_device = None
    for dev in devices:
        for (adtype, desc, value) in dev.getScanData():
            if target_name in value:
                #print(f"Found device: {value}, address: {dev.addr}")
                target_device = dev
                break
        if target_device:
            break
    
    if not target_device:
        #print(f"Device with name {target_name} not found.")
        return None

    # Connect to the device
    try:
        #print(f"Connecting to device with address: {target_device.addr}")
        peripheral = Peripheral(target_device.addr, 'random')
        peripheral.setDelegate(MyDelegate())  # Set notification handler
        return peripheral
    except BTLEException as e:
        print(f"Failed to connect: {e}")
        return None

def discover_services(peripheral):
    services = peripheral.getServices()
    for svc in services:
        #print(f"Service UUID: {svc.uuid}")
        characteristics = svc.getCharacteristics()
        for char in characteristics:
            if (char.getHandle() == 17):
                #print("Find char 17")
                svc_sample = svc.uuid
                char_sample = char.uuid
            #print(f"  Characteristic UUID: {char.uuid}, Handle: {char.getHandle()}")
            descriptors = char.getDescriptors()
            '''for des in descriptors:
                print(f"  Descriptor UUID: {des.uuid}, Handle: {des.handle}")'''

def enable_characteristic_notifications(peripheral, char_handle):
    try:
        peripheral.writeCharacteristic(char_handle, b'\x01\x00')
        
        #print(f"Notifications enabled on characteristic {char_handle}")
    except Exception as e:
        print(f"Failed to enable notifications: {e}")

def write_sample(peripheral, char_handle, rate):
    try:
        peripheral.writeCharacteristic(char_handle, rate, withResponse=False)
        print("successfully write to sample rate")
        
    except Exception as e:
        print(f"Failed to write data: {e}")

def main():
    target_device_name = "Bluepio"  # Name of the BLE app on Android
    
    char_acc_handle = 15;
    char_sample_handle = 17;

    # Connect to the BLE Tool app
    peripheral = connect_device(target_device_name)
    if not peripheral:
        return

    # Discover services and characteristics
    discover_services(peripheral)

    # Enable notifications on the specific characteristic
    enable_characteristic_notifications(peripheral, char_acc_handle)

    # Keep the program running to receive notifications
    print("Waiting for notifications...")
    notification_counter = 0
    i = 0
    while True:
        if (i == 50):
            write_sample(peripheral, char_sample_handle, b'\x10\x00'); # 1 sec
            print("write sample rate 1s")
            
        if(i == 100):
            write_sample(peripheral, char_sample_handle, b'\x5A\x00'); # 10 sec
            print("write sample rate 10s")

        if(i == 200):
            write_sample(peripheral, char_sample_handle, b'\xB4\x00')
            print("write sample rate 20s")
            
        i = i+1
        
        if peripheral.waitForNotifications(1.0):  # Wait for notifications (timeout 1 
            continue


if __name__ == "__main__":
    main()