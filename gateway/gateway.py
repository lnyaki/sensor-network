import paho.mqtt.client as mqtt
import serial
import argparse

ASK_CONFIG = "A"
TEMP = "T"
ACCEL = "Z"
BATTERY = "B"
UPDATE_MODE = b"U\n"
PERIODIC_MODE = b"P\n"
STOP_SEND = b"X\n"
START_SEND = b"S\n"

TOPIC = {
    TEMP: "temperature",
    ACCEL: "accelerometer",
    BATTERY: "battery"
}
periodic_mode = 0
has_subscribers = 0

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("config")
    client.subscribe("$SYS/broker/subscriptions/count")


def on_message(client, userdata, msg):
    global periodic_mode
    global has_subscribers
    if msg.topic == "config":
        if msg.payload == b"periodic":
            periodic_mode = 1
            client.serial.write(PERIODIC_MODE)
        elif msg.payload == b"update":
            periodic_mode = 0
            client.serial.write(UPDATE_MODE)
    elif msg.topic == "$SYS/broker/subscriptions/count":
        n_client = int(msg.payload)
        print("Number of clients: %d" % n_client)
        if n_client <= 2:
            has_subscribers = 0
            client.serial.write(STOP_SEND)
        else:
            has_subscribers = 1
            client.serial.write(START_SEND)


def main(broker_url, serial_port, serial_baud):
    client = mqtt.Client()

    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(broker_url)

    ser = serial.Serial(serial_port, serial_baud, timeout=0.1)
    client.serial = ser

    while client.loop() == 0:
        if ser.in_waiting:
            data = ser.readline().decode().split()

            if data[0] == ASK_CONFIG:
                if periodic_mode == 1:
                    ser.write(PERIODIC_MODE)
                else:
                    ser.write(UPDATE_MODE)
                if has_subscribers == 1:
                    ser.write(START_SEND)
                else:
                    ser.write(STOP_SEND)
            else:
                topic = TOPIC.get(data[0], "other")
                client.publish(topic, " ".join(data[1:]))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--port", help="Serial port", default="/dev/ttyUSB0")
    parser.add_argument("-b", "--baud-rate", help="Baudrate for the serial connection", default=115200)
    parser.add_argument("-H", "--host", help="IP of the mqtt broker", default="localhost")
    args = parser.parse_args()
    main(args.host, args.port, args.baud_rate)
