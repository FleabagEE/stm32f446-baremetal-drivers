import serial
import time

ser = serial.Serial('COM6', 115200, timeout=1)
time.sleep(0.2)
ser.reset_input_buffer()


def run_echo_test(name, data):
    ser.reset_input_buffer()
    ser.write(data)
    response = ser.read(len(data))

    if response == data:
        print(f"PASS: {name}")
    else:
        print(f"FAIL: {name}, sent {data}, got {response}")


def run_spi_test():
    ser.reset_input_buffer()
    ser.write(b'S')
    response = ser.read(1)

    if response == b'\xAB':
        print("PASS: spi loopback")
    else:
        print(f"FAIL: spi loopback, expected b'\\xAB', got {response}")


run_echo_test("single byte echo", b'A')
run_echo_test("multi byte echo", b'Hello')
run_spi_test()

ser.close()