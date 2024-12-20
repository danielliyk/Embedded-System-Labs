import asyncio
import struct
from bleak import BleakScanner, BleakClient, BleakError
from multiprocessing import shared_memory
import numpy as np
import matplotlib.pyplot as plt
from collections import deque

# Shared memory setup
try:
    existing_shm = shared_memory.SharedMemory(name="my_shared_mem")
    existing_shm.close()
    existing_shm.unlink()
except FileNotFoundError:
    pass  # No existing shared memory found, which is fine

# Now create the new shared memory block
shm = shared_memory.SharedMemory(create=True, size=4, name="my_shared_mem")

# Delegate to handle notifications
class MyDelegate:
    def __init__(self):
        pass

    # This method is called when a notification is received
    def handleNotification(self, char, data):
        # Assuming data format: 1 byte representing the value (either 0 or 1)
        if len(data) >= 1:
            value = struct.unpack('<B', data[:1])[0]  # Unpack the first byte as an unsigned 8-bit integer
            print(f"Received value: {value}")

            # Write the received value to shared memory
            np_array = np.ndarray(shape=(1,), dtype=np.int8, buffer=shm.buf)
            np_array[0] = value
            print(f"Wrote value: {value}")
            # Optional: Write the received value to a text file, overwriting the previous one
            '''with open("received_value.txt", "w") as file:
                file.write(str(value))'''
        else:
            print(f"Unexpected data length: {len(data)}. Data: {data.hex()}")
            # If nothing is received, clear the shared memory and the text file
            np_array = np.ndarray(shape=(1,), dtype=np.int8, buffer=shm.buf)
            np_array[0] = -1  # Clear shared memory value (-1 indicates no value)
            with open("received_value.txt", "w") as file:
                file.write("")

async def connect_device(target_name, timeout=10):
    scanner = BleakScanner()
    devices = await scanner.discover()  # Discover devices
    
    # Print all discovered devices
    print("Discovered devices:")
    for dev in devices:
        print(f"Name: {dev.name}, Address: {dev.address}")

    # Find device by name
    target_device = None
    for dev in devices:
        if dev.name and target_name in dev.name:  # Check if dev.name is not None
            print(f"Found device: {dev.name}, address: {dev.address}")
            target_device = dev
            break
    
    if not target_device:
        print(f"Device with name {target_name} not found.")
        return None

    # Attempt to connect to the device with a longer timeout and explicit address type
    try:
        print(f"Connecting to device with address: {target_device.address}")
        client = BleakClient(target_device.address, address_type='random')  # Explicit random address type
        
        # Attempt to connect with longer timeout
        is_connected = await asyncio.wait_for(client.connect(), timeout=timeout)
        
        if is_connected:
            client.set_disconnected_callback(lambda client: print(f"Disconnected from {target_device.address}"))
            return client
        else:
            print(f"Failed to connect to {target_device.address}")
            return None
    except asyncio.TimeoutError:
        print(f"Connection timed out after {timeout} seconds.")
        return None
    except BleakError as e:
        print(f"Failed to connect: {e}")
        return None

async def discover_services(client):
    services = await client.get_services()
    for svc in services:
        print(f"Service UUID: {svc.uuid}")
        for char in svc.characteristics:
            if char.handle == 13:
                print("Found char 13")
                global svc_sample, char_sample
                svc_sample = svc.uuid
                char_sample = char.uuid
            print(f"  Characteristic UUID: {char.uuid}, Handle: {char.handle}")
            for des in char.descriptors:
                print(f"  Descriptor UUID: {des.uuid}, Handle: {des.handle}")

async def enable_characteristic_notifications(client, char_uuid):
    try:
        await client.start_notify(char_uuid, MyDelegate().handleNotification)
        print(f"Notifications enabled on characteristic {char_uuid}")
    except Exception as e:
        print(f"Failed to enable notifications: {e}")

async def main():
    target_device_name = "BlueNRG"  # Name of the BLE app on Android
    char_acc_handle = 13

    # Connect to the BLE Tool app on Android with longer connection timeout
    client = await connect_device(target_device_name, timeout=15)  # Timeout set to 15 seconds
    if not client:
        return

    # Discover services and characteristics
    await discover_services(client)

    # Enable notifications on the specific characteristic
    await enable_characteristic_notifications(client, char_acc_handle)

    # Keep the program running to receive notifications
    print("Waiting for notifications...")

    while True:
        await asyncio.sleep(1)  # Wait for a short time to allow notification processing

if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.create_task(main())  # Run the main BLE task
    loop.run_forever()

    # Cleanup shared memory on exit
    shm.close()
    shm.unlink()
