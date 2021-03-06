# AUGC Tank Battle


This repository is for the AUGC (Arduino User Group Cagliari) Tank Battle project.
It is based on these two project:
+ **[SMARS modular robot](https://www.thingiverse.com/thing:2662828)**
+ **[Tank Go!](https://www.thingiverse.com/thing:2770957)**

Some mods are applied (for example, we use the NodeMCU 1.0 board in spite of Linkit 7697 board) in order to lower the total cost and applying little enhancements.

***Actually the project is in a alpha release: it work but not all functionalities are implemented.***

## Index

+ [To do list](#To-do-list)
+ [BOM (Bill of Materials)](#BOM-Bill-of-Materials)
+ [Printing instruction](#Printing-instruction)

## Tank functionalities
Here all the functionalities actually implemented.
+ **Wifi/Blynk connection**. If the tank cannot connect to the WiFi network or to the Blynk server (custom or official one), it will start the hotspot (SSID and password customizable) and shake the turret 3 times (NAK emote - configurable). Once connected to the hotspot, a captive portal will be displayed and it is possible to configure:
  + the WiFi credentials
  + the tank hotspot SSID and optionally the password
  + the Blynk server address (URL or IP), the Blynk server port and the Blynk token).
  
  If a connection to the Blynk server will be established, the tank shake the turret once (ACK emote - configurable)
+ **Battery management**. If the battery voltage go below a threshold (3.5V - customizable) the thank will move anymore. If the voltage raise up, the tank will move again.
+ **Ammo management**. There are several functionalities:
  + *recharging time*. Every time the tank shoot a round, it must wait a while (1500 millisecond - customizable) after shooting again. 
  + *max ammos*. The tank starts with a limited ammo quantity (20 - customizable). 
  + *current ammos*. Every time the tank shoot an ammo, the current ammos decrease. When the current ammos are zero, the tank will shoot anymore.
  + *auto regenerating ammos*. The ammos will regenerating every 7000 milliseconds (customizable). In order to regenerate ammos the tank needs to not fire. If fire a round, the regenerating timer will reset.
+ **Damage management**. There are several functionalities:
  + *max damage*. The total hit point of the tank (200 - customizable).
  + *ammo damage*. The damage of a single shoot (10 - customizable).
  + *current damage*. Every time the tank will be hit, the current damage will increased by the ammo damage. If the current damage reach the total hit points (max damage) the tank will move anymore. In order to move again, you have to repair the tank (see below)
  + *repair tank*. If the tank max out the total damage (no more hit points) the repair button will be enabled. In order to repair the tank and get moving it again, quickly press repeatedly the repair button to empty the damage bar.
+ **Moving management**. The tank can move itself and its turret using the custom Blynk app. If the voltage of the battery is below a threshold (see battery management) or if the tank is damaged (see damage management) the tank will no move.
+ **Configuration**. in the "CONFIG" tab of the custom Blynk app it is possible to configure the leftmost,  the rightmost and the center turret position.

## To do list
#### Software related
+ [ ] Multiplayer platform
+ [ ] Audio fx
+ [ ] Drive with accelerometers
+ [x] WifiManager and custom personalization
+ [x] Damage routines
+ [x] Repair routines
+ [x] Discharging and recharging ammos
+ [x] Blynk interface
+ [x] Moving the tank
+ [x] Moving the turret
+ [x] Shoot (with ID)
+ [x] Receive (and decode) shooter ID

#### Repository related
+ [ ] Tools/Hardware section
+ [ ] Arduino IDE/library configuration
+ [ ] Assembling section
+ [x] 3D printing section
+ [x] BOM Section
+ [x] Creating and populating repository

#### 3D files related
+ [ ] Upgrade the IR turret cover and the IR cover to accomodate a wireless camera for FPV playing.
+ [ ] Modify the Track_32PCS.stl file to slightly increase the center joint hole diameter and reduce the lateral joint hole diameter.
+ [x] IR_Cover.stl - Make a little socket/hetch for the IR Receiver terminals. Make some wings to keep in place the cover.

#### Harware related
+ [ ] Circuit for driving more current to the IR diode. 
+ [ ] Circuit for driving two IR diodes (dual cannon turret "Turret_X.stl").

## BOM (Bill of Materials)
Here what you need to build one:
+ **NodeMCU 1.0** - *Pay attention* that there are several models of NodeMCU: they can differ from the **USB to Serial chip** (CP2102 or CH340G) and/or from the **size**. You can identify which NodeMCU is ok by looking at the pin labels: a good ones have the pin labels printed all in the same row ([see this image](https://github.com/shurillu/AUGC_Tank_Battle/blob/master/images/NodeMCU_Comparison.jpg)). [I have this one](https://www.aliexpress.com/item/Nodo-MCU-bordo-di-Sviluppo-di-Kit-V3-CP2102-NodeMCU-Motor-Shield-Wifi-Esp8266-Esp-12e/32953905540.html)
+ **NodeMCU 1.0 Motor Shield** - It use the L293D driver and **MUST** be compatible with the NodeMCU 1.0. In the previous link you can find both (NodeMCU and motor shield).
+ **N20 6 Volt DC motor** - *You need two of them*. I use the 200 RPM version (the tank climbs obstacles with no hassle and is still speedy). Feel free to experiments with other RPMs. [Here an example](https://www.aliexpress.com/item/Spedizione-Gratuita-DC-3-v-6-v-12-v-N20-Mini-Micro-Metal-Gear-Motore-con/32953037195.html). 
+ **9g Servo Motor** - A 9g Servo motor for turret movement. You can choose between plastic or metal geared servo, it's up to you. If you buy the cheap chinese version, my advice is to buy several because sometimes they are faulty. [Here an example](https://www.aliexpress.com/item/100-NUOVO-Commercio-All-ingrosso-SG90-9g-Micro-Servo-Motore-Per-Robot-6CH-RC-Elicottero-Aereo/32831149040.html).
+ **5V 2A step up/boost converter** - There are several models, some with fixed output voltage (smaller in size), other with a voltage adjust potentiometer (bigger in size). If you choose the boost converter with the voltage adjust you can squeeze more power from the motors (setting the output voltage to 6 Volt!).  [I have this one](https://www.aliexpress.com/item/DC-DC-Auto-Boost-Buck-adjustable-step-down-Converter-XL6009-Module-Solar-Voltage/32639790122.html).
+ **TSOP38238 IR receiver** - Infrared receiver with IR filter and embedded demodulator. You can buy it [here](https://www.aliexpress.com/item/10-pz-100-nuovo-e-originale-TSOP38238-Ricevitore-IR-I-Moduli-per-I-Sistemi-di-Controllo/32947920639.html) or at your local retail seller (or wherever you want).
+ **5mm 940nm 50mA IR diode transmitter** - There are several models, I choose a model with an irradiation angle of 20 degree, in order to concentrate the irradiation power on the front of the diode. If you want to use the "Turret_X.stl" turret, you need two of this diode.
+ **100K Ohm 1/4W 1% resistor** - Used as a voltage divider, we need it to read the battery voltage.
+ **180 Ohm 1/4W 5% resistor** - Used as a current limiter for the IR diode. If you want to use the "Turret_X.stl" turret, you need two of this resistor.
+ **1S LiPo battery** - The tank main power source, 600mA 5C at least. Max dimensions are 55mm x 40mm x 9mm. [This one could be a good choice - not tested yet](https://hobbyking.com/en_us/turnigy-nano-tech-750mah-1s-70c-lipo-pack-jst-walker-hr-tech.html).
+ **Battery connectors** - A connector that match with the battery connector. *You need two of them* (one for the tank, optionally one for the battery charger). For example, if you choose the battery as above, [you need these](https://www.aliexpress.com/item/2-10-Pairs-100-150mm-2-Spille-Connettore-JST-Spina-del-Cavo-Maschio-Femmina-Per-RC/32870752993.html).
+ **LiPo charger** - (optional) A little charger for the LiPo. A good choice is a charger with the TP4056 chip. [Here a good one](https://www.aliexpress.com/item/Smart-Electronics-5V-Micro-USB-1A-18650-Lithium-Battery-Charging-Board-With-Protection-Charger-Module-for/32500042672.html).
+ **Dupont pin header connector kit** - A kit to make male/female dupont connectors (2.54 mm). [Here a good one](https://www.aliexpress.com/item/620-pz-Dupont-Connector-2-54mm-Dupont-Cavo-Ponticello-linea-di-Spille-Header-Kit-di-Alloggiamento/32950939016.html). Alternatively you can buy already crimped cables [like these](https://www.aliexpress.com/item/Dupont-line-120pcs-10cm-male-to-male-male-to-female-and-female-to-female-jumper-wire/32825558073.html). Pay attention to choose one with at least a *female connector* and *30cm long*.
+ **M3 nuts** - *You need two of them*, used in conjunction with the grub screw to bolt the main wheels. You can find it at your harware local reseller or buy a [kit like that (buy the cap/socket head kit)](https://www.aliexpress.com/item/250pc-set-A2-Stainless-Steel-M3-Cap-Button-Flat-Head-Screws-Sets-Hex-Socket-Bolt-With/32811514698.html).
+ **M3 6mm grub screw** - *You need two of them*, used in conjunction with the nuts to bolt the main wheels. You can find it at your harware local reseller or buy a [kit like that](https://www.aliexpress.com/item/50-pz-M3x6mm-Bullone-di-Fissaggio-In-Acciaio-Al-Carbonio-autofilettanti-Vite-A-Esagono-Incassato-Grub/32669329846.html).
+ **M2 8mm screw** - *You need eight of them*, better if they are the self tapping version. Used to bolt together the tank chassis. You can find it at your harware local reseller or buy a [kit like that](https://www.aliexpress.com/item/100-pz-DIN7982-M1-4-M1-7-M2-M2-3-M2-6-KA-Elettronico-Piccole-Viti/32955835312.html).
+ **Heat shrink tube** - Useful for insulate soldered terminals like IR receiver terminals, IR transmitter terminals. [A good one kit](https://www.aliexpress.com/item/328-Pz-8-Formati-Multi-Colore-Poliolefine-2-1-Halogen-Free-Tubi-Termorestringenti-Tubo-Assortimento-Manicotti/32891366605.html).
+ **DFPlayer module** - Optionally, used to implement the super cool audio effects. [I have this one](https://www.aliexpress.com/item/LEORY-DFPlayer-Mini-Lettore-MP3-Modulo-Vocale-Modulo-per-Arduino-bit-DAC-Uscita-Supporto-MP3-WAV/32849088916.html).
+ **MicroSD** - Optionally, used to store the MP3 sound fx. Any size is fine, up to 32GB.
+ **Mini Speaker 3mm ~ 29mm, 8 Ohm, 0.25W ~ 1W** - Optionally, a speaker for playing the sound fx. [something like that - not tested yet](https://www.aliexpress.com/item/2-PCS-1-w-eight-o-tablet-horn-the-original-way-N90-U9GT2-1420-panel-speaker/32782733427.html).

## Printing instruction

In this [section](https://github.com/shurillu/AUGC_Tank_Battle/tree/master/3D%20Files#3D-printing-instruction) you can find some instructions/advices for printing all the tanks parts.



