import paho.mqtt.client as mqtt
import datetime

f = open("data.csv", "a")
out_temperature = None

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("Valerio/#")
    client.subscribe("fuori_come_un_balcone/temp")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global out_temperature
    if msg.topic == "Valerio/myRoomTemperature" and out_temperature is not None:
        row = create_row(msg)
        print(row)
        f.write(row)
        f.flush()
    elif msg.topic == "fuori_come_un_balcone/temp":
        out_temperature = msg.payload.decode("utf-8")


def create_row(msg):
    global out_temperature
    dt = datetime.datetime.now()
    now = dt.strftime("%d/%m/%Y,%H:%M")
    room_temperature = msg.payload.decode("utf-8")
    return "{},{},{}\n".format(now, room_temperature, out_temperature)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("mqtt.atrent.it", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()