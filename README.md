### AirController

*Nome Progetto*: AirController

*Autore*: Valerio Cislaghi

*Descrizione*: AirController è un sistema intelligente di controllo automatico per condizionatori che hanno rotto (o non hanno) un sensore di temperatura (quindi portando sempre la temperatura/umidità a livelli minimi possibili).
L'obbiettivo è gestire il condizionatore attraverso segnali inviati tramite infrarossi.
AirController riceve in input un indice di calore come target (HUMIDEX) e il suo obbiettivo sarà mantenere costante questo indice.
il controller si basa su diversi fattori:
- Temperatura/Umidità esterna (chiamate API a openWeather).
- Temperatura/Umidità interna (DHT22/DHT11).
- Numero di persone nella stanza (AM312), ancora da testare.

Altri componenti utilizzati:
- 2 diodi a infrarossi a LED (emissione e ricezione).
- Display che mostra la temperatura interna.

Tutti i dati (temperatura, unimdità, indice di calore, conta persone ecc...) saranno inviati ad un server MQTT per la sincronizzazione dei vari ESP8266 e per la raccolta dei dati, per stabilire (attraverso analisi) se effettivamente c'è un risparmio di corrente e di salute.

*Link a repo*: https://github.com/Jaivra/AirController

*Licenza Scelta*: GPLv3

*Data presentazione*: fine Luglio

