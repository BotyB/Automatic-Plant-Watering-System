# Automatic Plant Watering System

![Plant Watering System](IMG_1338.jpg)

This project is an automatic watering system designed to take care of up to four plants independently.

Each plant has its own soil moisture sensor and water pump. The system constantly checks how dry the soil is and only waters the plant that actually needs it. This means every plant receives water based on its own condition rather than following a fixed watering schedule.

The system is controlled by an ESP32 microcontroller and uses a four-channel relay module to switch the water pumps on and off safely. Two water-level sensors are also used to monitor the reservoir and prevent the pumps from running when there is not enough water available.

The main goal of the project is to make plant care easier while avoiding both underwatering and unnecessary watering.

## Main Features

* Monitors the soil moisture of four plants separately
* Uses one dedicated water pump for each plant
* Automatically waters only the plants that are too dry
* Monitors the water reservoir level
* Prevents the pumps from running dry
* Can be monitored and controlled through Blynk
* Allows moisture thresholds to be adjusted for different plants
* Uses ventilation lines to prevent unwanted water siphoning after a pump stops

The system was built as a practical home automation project and can be expanded later with additional sensors, notifications, data logging, or more watering zones.

## Early Planning and Component Selection

The project started with a simple paper sketch showing the main idea: four plants, four soil sensors, four water pumps, and one central controller.

The first stage focused on choosing the correct parts and deciding how the system should be powered. Capacitive soil-moisture sensors were selected because they are more resistant to corrosion than traditional exposed-metal probes. A four-channel relay module was chosen so each water pump could be controlled independently.

Both 5V and 12V pumps were considered during planning. The final design uses low-voltage pumps and a single wall power supply rated for enough current to operate the controller and pumps safely. This avoided the need for several separate adapters. Buck converters were considered in case different parts required different voltages, but the final power arrangement was kept as simple as possible.


## Reservoir, Controller and Safety Design

A suitable water container was selected based on capacity, stability, and ease of refilling. Two water-level protection methods were added: one sensor reports the tank status to the ESP32, while a second hardware cutoff prevents the pumps from running when the water level becomes too low.

An ESP32 development board was chosen as the main controller because it provides enough input and output pins, built-in Wi-Fi, analog sensor support, and compatibility with Blynk. A standard ESP32 module with its built-in antenna was sufficient for this installation, so an external U.FL antenna was not required.

The electronic section of each soil sensor was protected against moisture using liquid electrical tape or silicone, while the sensing section remained exposed to the soil.

The final system includes several safety and warning features:

* An empty-tank warning light
* Software protection that blocks watering when the tank is empty
* A separate hardware cutoff in the pump power line
* Individual watering intervals to reduce overwatering
* One-pump-at-a-time operation to limit power consumption
* Manual control that still respects the empty-tank safety system

