import asyncio
from bleak import BleakScanner, BleakClient

async def scan_and_select_device():
    devices = await BleakScanner.discover()
    print("Available devices:")
    for i, device in enumerate(devices):
        print(f"{i}: {device.name} ({device.address}) - RSSI: {device.rssi} dB")

    device_number = int(input("Enter your device number: "))
    return devices[device_number]

async def print_services_and_characteristics(device):
    async with BleakClient(device) as client:
        print(f"Connected to {device.name}")
        services = await client.get_services()
        print("Services and characteristics:")
        for service in services:
            print(f"Service {service.uuid} - {service.description}")
            for char in service.characteristics:
                print(f"  Characteristic {char.uuid} - {char.description}")
                for desc in char.descriptors:
                    print(f"    Descriptor {desc.uuid} - {desc.description}")

async def main():
    device = await scan_and_select_device()
    await print_services_and_characteristics(device)

if __name__ == "__main__":
    asyncio.run(main())
