# Automatic Plant Watering System

![Automatic Plant Watering System](IMG_1337.jpg)

![Completed Plant Watering System](IMG_1338.jpg)

This project is an automatic watering system designed to care for up to four plants independently.

Each plant has its own capacitive soil-moisture sensor and dedicated water pump. Instead of watering every plant on a fixed schedule, the system checks the condition of each pot and waters only the plant whose soil has become too dry.

The system is controlled by an ESP32 microcontroller and uses a four-channel relay module to operate the pumps. Two water-level protection systems monitor the reservoir and prevent the pumps from running when there is not enough water available.

![Watering System Installation](IMG_1111.jpg)

The main goal was to create a practical system that reduces manual plant care while avoiding both underwatering and unnecessary repeated watering.

## Main Features

* Monitors the soil moisture of four plants separately
* Uses one dedicated water pump for each plant
* Automatically waters only the plants that are too dry
* Monitors the water reservoir level
* Prevents the pumps from running dry
* Can be monitored and controlled through Blynk
* Allows moisture thresholds to be adjusted for different plants
* Uses ventilation lines to prevent unwanted water siphoning after a pump stops
* Uses one pump at a time to reduce peak current
* Includes both software and hardware water-level protection
* Provides visual warning patterns through a front status LED

The system was built as a practical home automation project and can be expanded later with additional sensors, notifications, data logging, or more watering zones.

## 1. Project Idea

The original idea was to build one controller capable of caring for four separate plants.

Each plant would have:

* One soil-moisture sensor
* One dedicated water pump
* One separate water hose
* Its own moisture threshold
* Its own watering interval

![Reservoir Testing](IMG_1314.jpg)

![Reservoir Hose Arrangement](IMG_1315.jpg)

![Modified Reservoir](IMG_1316.jpg)

This allows plants with different water requirements to be monitored independently instead of treating all four pots as one watering zone.

## 2. Initial Planning

![Original Paper Wiring Sketch](IMG_1347.jpg)

The project began with a paper sketch showing the complete electrical layout before any permanent wiring was made.

The design used one main 5V power source divided into two functional branches.

### Logic Branch

The logic branch supplies the low-current electronics:

* ESP32 microcontroller
* Four capacitive soil-moisture sensors
* Four-channel relay control circuit
* Front status LED
* Water-level status input

The four soil sensors are powered from the ESP32's 3.3V output. Their analog signal wires are connected to GPIO pins:

* Sensor 1: GPIO 32
* Sensor 2: GPIO 33
* Sensor 3: GPIO 34
* Sensor 4: GPIO 35

These pins belong to ADC1, which means they can still be used while the ESP32 Wi-Fi connection is active.

The four relay-control inputs are connected to:

* Relay 1: GPIO 16
* Relay 2: GPIO 17
* Relay 3: GPIO 18
* Relay 4: GPIO 19

The warning LED is connected to GPIO 25 through a current-limiting resistor of approximately 220–330 ohms.

### Pump-Power Branch

The second branch supplies the higher-current side of the system:

* Four 5V water pumps
* Relay common terminals
* Hardware water-level cutoff
* Main pump-power path

Separating the logical control section from the higher-current pump branch made the system easier to understand and reduced the chance of pump current affecting the ESP32.

Both branches still share the same 5V supply and common ground. They are functionally separated, not electrically isolated.

## 3. Component Selection

![Pumps, Sensors and Hoses](IMG_1351.jpg)

*Four capacitive soil sensors, four 5V water pumps and watering hoses.*

![Four-Channel Relay](IMG_1353.jpg)

![Four-Channel Relay With Case](IMG_1287.jpg)

*Four-channel relay module used to control the pumps independently.*

![Original Reservoir](IMG_1350.jpg)

*The original taller water reservoir considered during the first build.*

The first stage involved choosing the sensors, pumps, relay module and operating voltage.

Capacitive soil-moisture sensors were selected instead of exposed-metal resistive probes. Capacitive sensors are generally more suitable for long-term installation because the sensing surface does not rely on two exposed metal electrodes that gradually corrode in wet soil.

A four-channel relay module was selected because it allows the ESP32 to switch each pump independently while keeping pump current away from the ESP32 GPIO pins.

![Original Reservoir Mounted Components](IMG_1335.jpg)

Both 12V and 5V pumps were considered. The final system uses 5V pumps, allowing the ESP32, relay module and pumps to operate from one suitably rated 5V wall power supply.

![Original Reservoir Mounted Components Back](IMG_1340.jpg)

![Original Reservoir Mounted Components Back1](IMG_1341.jpg)

![Original Reservoir Mounted Components Back2](IMG_1342.jpg)

![Original Reservoir Mounted Components Back3](IMG_1343.jpg)

A power supply rated for approximately 3A was selected to provide enough current for the controller and pumps. Because the final components all operate from compatible voltages, additional buck converters were not required.

Only one pump is operated at a time. This reduces the peak load on the power supply and prevents several pumps from starting simultaneously.

## 4. Water Reservoir and Siphon Problem


![Front Status LED](IMG_1432.jpg)

![Front Status LED](IMG_1444.jpg)

![Completed Front Panel](IMG_1447.jpg)


The first reservoir was relatively tall. During testing, its water level could remain higher than the ends of the watering hoses.

This created a siphoning problem. After a pump stopped, water could continue moving through the hose because of the height difference between the reservoir and the plant pots. In some cases, this allowed more water to leave the reservoir than intended and created a risk of overflowing the pots.

The taller bucket was therefore replaced with a shorter container. Lowering the reservoir reduced the vertical pressure difference and made unwanted gravity-fed water flow less likely.

Changing the bucket alone improved the system, but it did not fully guarantee that every hose would stop flowing immediately.

## 5. 3D-Printed Flow Breakers and Vent Lines

To fully stop unwanted siphoning, custom 3D-printed flow breakers were added near the ends of the watering hoses.

![3D Printed Flow Breaker](IMG_1431.jpg)

![3D Printed Flow Breaker Plant](IMG_1444.jpg)

Each outlet includes a curved vent section that allows air to enter the hose after the pump stops. Introducing air breaks the continuous water column inside the hose and interrupts the siphon effect.

Without the vent, a hose filled completely with water can continue drawing water from the reservoir even though the pump is no longer powered.

The final arrangement provides two forms of siphon protection:

* A shorter reservoir with less height above the plants
* Vented 3D-printed hose outlets that break the water column

This ensures that each plant receives approximately the intended pump dose rather than continuing to receive water through gravity flow.

## 6. Controller and Wireless Connection

![ESP32-U Controller](IMG_1352.jpg)

An ESP32-U development board was selected as the main controller.

The ESP32 provides:

* Built-in Wi-Fi
* Enough GPIO pins for four sensors and four relays
* Multiple ADC1 analog inputs
* Blynk compatibility
* Timers and automatic watering logic
* Remote monitoring and manual control

The ESP32-U version was chosen because it supports an external antenna through a U.FL connector.

Although a regular ESP32 module with a PCB antenna may be sufficient in many indoor installations, the external antenna provides more freedom when mounting the controller inside an enclosure or near objects that can weaken the Wi-Fi signal.

The antenna can be positioned outside or away from the main electronics enclosure, improving the reliability of the connection to the router.

Some ESP32 boards that include both a PCB antenna and a U.FL connector require a small hardware antenna-selection change. This should be checked for the exact board model before connecting an external antenna.

## 7. Soil Sensor Protection

![Protected Soil Sensor Electronics](IMG_1112.jpg)

![Protected Soil Sensor Electronics](IMG_1443.jpg)

Only the lower sensing section of a capacitive soil sensor is intended to remain inside the soil.

The upper electronic section contains exposed components and solder joints that can be damaged by water, condensation or wet soil. This section was protected using liquid electrical tape or silicone sealant.

The protective coating was applied around:

* The electronic components
* Solder joints
* Cable connection
* Upper edge of the circuit board

The active sensing area was left uncovered so the sensor could continue measuring changes in soil moisture correctly.

Care was also taken not to bury the electronic section below the soil line.

## 8. Safety and Warning Systems

The system includes both software and hardware protection.

### Software Water-Level Protection

One float sensor is connected to the ESP32. The program checks this sensor before starting a pump.

When the tank is empty:

* Automatic watering is blocked
* Manual watering is blocked
* An error is shown in Blynk
* The front warning LED begins flashing
* Any active pump is stopped

### Independent Hardware Cutoff

A second water-level switch is installed directly in the pump-power branch.

This switch physically disconnects power from the pumps when the water level becomes too low. Because this protection does not depend on the ESP32 or software, it can still stop the pumps if:

* The ESP32 freezes
* Wi-Fi disconnects
* Blynk becomes unavailable
* A software error occurs
* The status sensor fails to be processed correctly

The hardware switch acts as the final dry-run protection for the pumps.

### One-Pump-at-a-Time Protection

The program permits only one pump to operate at a time.

This:

* Reduces peak current
* Prevents simultaneous watering
* Makes pump activity easier to track
* Reduces stress on the power supply

### Watering Interval Protection

After automatic watering, the affected zone must wait before it is eligible for another automatic dose.

This gives water time to spread through the soil and reach the moisture sensor. Without this delay, the system could water repeatedly while the sensor still temporarily reports dry soil.

## 9. Front Status LED


![Controller Enclosure](IMG_1445.jpg)

![Internal Electronics](IMG_1446.jpg)


A front-mounted status LED provides basic information without requiring the Blynk application to be opened.

The current warning patterns are:

* Normal operation: LED remains off
* Pump active: two short flashes repeat every second
* Tank empty: LED flashes rapidly and continuously

The empty-tank pattern remains active until the reservoir is refilled and the float sensor returns to the normal position.

The same conditions are also reported through the Blynk dashboard.

## 10. Blynk Web and Mobile Dashboard

The system can be monitored and controlled through Blynk on both desktop and mobile devices.

**Mobile Dashboard**

![Blynk Mobile Dashboard](IMG_1426.jpg)

![Blynk Mobile Dashboard](IMG_1428.jpg)

![Blynk Mobile Dashboard](IMG_1429.jpg)

**Web Dashboard**

![Blynk Web Dashboard](IMG_1451.jpg)

### Dashboard Features

* Displays the moisture level of all four plants from 0% to 100%
* Allows individual manual watering
* Shows whether a pump is currently active
* Displays the remaining safety interval before automatic watering is allowed again
* Shows water-tank status
* Reports errors and blocked watering conditions
* Provides status indicators for normal operation, watering and an empty reservoir

Manual watering ignores the scheduled watering hours and normal automatic watering interval. However, it does not bypass the empty-tank protection.

### Moisture Calibration

The ESP32 internally reads each sensor using a raw ADC range from 0 to 4095. These readings are converted into a simpler percentage for the dashboard.

Default calibration values are approximately:

* 1200: very wet
* 2000: moderately wet
* 2600 or more: becoming dry
* 3200: very dry

With the default conversion:

* 100% means very wet
* Approximately 50% means medium moisture
* 0% means very dry

Every capacitive sensor can produce slightly different values, so wet and dry calibration should be performed for the actual sensors and soil being used.

### Status Indicators

* Normal: status LED remains off
* Watering: two short flashes repeat every second
* Tank empty: the LED flashes rapidly and continuously
* Pump active: the Blynk pump indicator turns on while one of the pumps is running
* Error message: reports an empty tank, another active pump, watering outside the permitted hours, or missing clock synchronization


## 11. Software Setup

1. Download or clone the GitHub repository.
2. Copy `secrets.example.h`.
3. Rename the copy to `secrets.h`.
4. Open `secrets.h` and replace the placeholder values with your own credentials:

```cpp
#define BLYNK_TEMPLATE_ID   "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

#define WIFI_SSID "YOUR_WIFI_NETWORK_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
```

5. Keep `secrets.h` private. It is excluded from Git through `.gitignore`.
6. Open `PlantWaterer.ino` in Arduino IDE.
7. Install the ESP32 board package and Blynk library if they are not already installed.
8. Select the correct ESP32 board and COM port.
9. Upload the sketch to the ESP32.
10. Configure each Blynk moisture datastream with a minimum value of 0 and a maximum value of 100.

## 12. Calibration and Use

The default sensor calibration in the code is:

```cpp
constexpr int RAW_WET = 1200;
constexpr int RAW_DRY = 3200;
```

These values are starting points only.

For better accuracy:

1. Record the sensor reading in very wet soil.
2. Record the sensor reading in dry soil.
3. Replace `RAW_WET` and `RAW_DRY` with the measured values.
4. Repeat testing for all four sensors.
5. Adjust each plant's target moisture percentage if necessary.
6. Adjust the pump run time for each zone according to pot size, hose flow and plant needs.

The system should be observed during initial testing to confirm that:

* Each relay controls the correct pump
* Each sensor is assigned to the correct plant
* The tank sensors react correctly
* The pumps stop when the reservoir is empty
* The vented hose outlets break siphoning
* The Blynk readings match the physical system
* The watering dose is suitable for each pot
