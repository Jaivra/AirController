import paho.mqtt.client as mqtt
import datetime


class Measure:
    def __init__(self, topic):
        self._topic = topic
        self._temperature = None
        self._humidity = None
        self._heat_index = None


    def has_sub_topic(self, topic):
        return self._topic in topic

    def update_measure(self, desc, value):
        if desc == "temperature":
            self._temperature = value
        elif desc == "humidity":
            self._humidity = value
        else:
            self._heat_index = value

    @property
    def temperature(self):
        return self._temperature

    @property
    def humidity(self):
        return self._humidity

    @property
    def heat_index(self):
        return self._temperature

    def has_data(self):
        return self._temperature is not None and self._humidity is not None and self._heat_index is not None

    def __repr__(self):
        return "Temperatura: {} - Umidità: {} - Indice di calore: {}".format(self._temperature, self._humidity, self._heat_index)

    def to_csv(self):
        return "{},{},{}".format(self._temperature, self._humidity, self._heat_index)

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("valerio/#")


def on_message(client, userdata, msg):
    global room_measure, out_measure

    topic = msg.topic
    if out_measure.has_sub_topic(topic):
        out_measure.update_measure(topic.split('/')[-1], float(msg.payload.decode("utf-8")))
    elif room_measure.has_sub_topic(topic):
        room_measure.update_measure(topic.split('/')[-1], float(msg.payload.decode("utf-8")))
        if room_measure.has_data() and out_measure.has_data():
            dt = datetime.datetime.now()
            now = dt.strftime("%d/%m/%Y,%H:%M")
            row = "{},{},{}\n".format(now, room_measure.to_csv(), out_measure.to_csv())
            #print("****", row)
            f.write(row)
            f.flush()

f = open("data.csv", "a")
out_measure = Measure("valerio/out")
room_measure = Measure("valerio/room")

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

mqtt_client.connect("mqtt.atrent.it", 1883, 60 * 10)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
mqtt_client.loop_forever()
