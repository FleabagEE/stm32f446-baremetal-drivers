import serial

ser = serial.Serial('COM6', 115200, timeout=1)

def run_test(name, data):
    ser.write(data)
    response = ser.read(len(data))
    if response == data:
        print(f"PASS: {name}")
    else:
        print(f"FAIL: {name}, sent {data}, got {response}")

test_cases = [
    ("single byte", b'A'),
    ("multi byte", b'Hello'),
]

for name, data in test_cases:
    run_test(name, data)


ser.close()

