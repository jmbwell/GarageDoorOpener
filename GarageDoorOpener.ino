/*
 * GarageDoorOpener.ino
 *
 *  Created on: 2020-06-27
 *      Author: jmbwell (John Burwell)
 *
 * HAP section 8.16 garage door opener
 * An accessory that contains a garage door opener that opens and closes
 *
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);


//
// GPIO assignments
//

#define PIN_OPERATOR_CONTROL 14 // NodeMCU pin D5
#define PIN_SENSOR_CLOSED     5 // NodeMCU pin D1
#define PIN_SENSOR_OPENED     4 // NodeMCU pin D2


//
// Position values
//

#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN 0
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED 1
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING 2
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING 3
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED 4
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN 255

#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN 0
#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED 1
#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_UNKNOWN 255

#define HOMEKIT_CHARACTERISTIC_LOCK_CURRENT_STATE_UNSECURED 0
#define HOMEKIT_CHARACTERISTIC_LOCK_CURRENT_STATE_SECURED 1
#define HOMEKIT_CHARACTERISTIC_LOCK_CURRENT_STATE_JAMMED 2
#define HOMEKIT_CHARACTERISTIC_LOCK_CURRENT_STATE_UNKNOWN 3

#define HOMEKIT_CHARACTERISTIC_LOCK_TARGET_STATE_UNSECURED 0
#define HOMEKIT_CHARACTERISTIC_LOCK_TARGET_STATE_SECURED 1
#define HOMEKIT_CHARACTERISTIC_LOCK_TARGET_STATE_JAMMED 2
#define HOMEKIT_CHARACTERISTIC_LOCK_TARGET_STATE_UNKNOWN 3


//
// HomeKit setup
//

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_current_door_state;
extern "C" homekit_characteristic_t cha_target_door_state;
extern "C" homekit_characteristic_t cha_obstruction_detected;
extern "C" homekit_characteristic_t cha_name;
extern "C" homekit_characteristic_t cha_lock_current_state;
extern "C" homekit_characteristic_t cha_lock_target_state;


// For reporting heap usage on the serial output every 5 seconds
static uint32_t next_heap_millis = 0;


//
// Interrupt handler 
// 

// Interrupts cannot be stored in flash; tell the compiler to put it in RAM
void ICACHE_RAM_ATTR handle_sensor_change(); 

// When one of the pins changes, toggle a variable to TRUE so we can respond inside the main loop()
bool sensor_interrupt = FALSE;
void handle_sensor_change() {
  sensor_interrupt = TRUE;
}


//
// Getters
//

// Called when getting current door state
homekit_value_t cha_current_door_state_getter() {

  // Stash the current state so we can detect a change
  homekit_characteristic_t current_state = cha_current_door_state;

  // Read the sensors and use some logic to determine state
  if ( digitalRead(PIN_SENSOR_OPENED) == LOW ) {
    // If PIN_SENSOR_OPENED is low, it's being pulled to ground, which means the switch at the top of the track is closed, which means the door is open
    cha_current_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN;
  } 
  else if ( digitalRead(PIN_SENSOR_CLOSED) == LOW ) {
    // If PIN_SENSOR_CLOSED is low, it's being pulled to ground, which means the switch at the bottom of the track is closed, which means the door is closed
    cha_current_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED;
  } else {
    // If neither, then the door is in between switches, so we use the last known state to determine which way it's probably going
    if (current_state.value.uint8_value == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED) {
      // Current door state was "closed" so we are probably now "opening"
      cha_current_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING;
    } else if ( current_state.value.uint8_value == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN ) {
      // Current door state was "opened" so we are probably now "closing"
      cha_current_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING;
    }

    // If it is traveling, then it might have been started by the button in the garage. Set the new target state:
    if ( cha_current_door_state.value.uint8_value == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING ) {
      cha_target_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN;
    } else if ( cha_current_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING ) {
      cha_target_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED;
    }
    // ... and then notify HomeKit clients
  	LOG_D("Target door state: %i", cha_target_door_state.value.uint8_value);
    homekit_characteristic_notify(&cha_target_door_state, cha_target_door_state.value);
  }

	LOG_D("Current door state: %i", cha_current_door_state.value.uint8_value);
	return cha_current_door_state.value;
}

// Called when getting current obstruction state
homekit_value_t cha_obstruction_detected_getter() {
	LOG_D("Obstruction: %s", cha_obstruction_detected.value.bool_value ? "Detected" : "Not detected");
	return cha_obstruction_detected.value;
}

// Called when getting current lock state
homekit_value_t cha_lock_current_state_getter() {
	LOG_D("Current lock state: %i", cha_lock_current_state.value.uint8_value);
	return cha_lock_current_state.value;
}


// 
// Setters
//

// Called when setting target door state
void cha_target_door_state_setter(const homekit_value_t value) {

  // State value requested by HomeKit
  cha_target_door_state.value = value;
	LOG_D("Target door state: %i", value.uint8_value);

  // If the current state is not equal to the target state, then we "push the button"; otherwise, we do nothing
  if (cha_current_door_state.value.uint8_value != cha_target_door_state.value.uint8_value) {
    digitalWrite(PIN_OPERATOR_CONTROL, HIGH);
    delay(500);
    digitalWrite(PIN_OPERATOR_CONTROL, LOW);
  }
  
}


//
// Setup and loop
//

void my_homekit_setup() {

	// Set the setters and getters
	cha_current_door_state.getter = cha_current_door_state_getter;
	cha_target_door_state.setter = cha_target_door_state_setter;
	cha_lock_current_state.getter = cha_lock_current_state_getter;
	cha_obstruction_detected.getter = cha_obstruction_detected_getter;

	arduino_homekit_setup(&config);
}

void my_homekit_loop() {

	arduino_homekit_loop();

	const uint32_t t = millis();
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d", ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
	}

}


void setup() {

  // Set the sensor pins to input with pullup resistor
  pinMode(PIN_SENSOR_CLOSED, INPUT_PULLUP);
  pinMode(PIN_SENSOR_OPENED, INPUT_PULLUP);

  // Set the control pin to output
  pinMode(PIN_OPERATOR_CONTROL, OUTPUT);

  // Set interrupts to watch for changes in the open/close sensors
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_CLOSED), handle_sensor_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_OPENED), handle_sensor_change, CHANGE);

  Serial.begin(115200);
  wifi_connect(); // in wifi_info.h
  //homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
  my_homekit_setup();

  // Initialize the current door state
  homekit_value_t current_state = cha_current_door_state_getter();
  homekit_characteristic_notify(&cha_current_door_state, cha_current_door_state.value);

  // Initialize target door state based on the current door state
  switch (cha_current_door_state.value.uint8_value) {
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED: 
      cha_target_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED; 
      break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING: 
      cha_target_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED; 
      break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN: 
      cha_target_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN; 
      break;
    case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING: 
      cha_target_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN; 
      break;
    default: 
      cha_target_door_state.value.uint8_value = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_UNKNOWN; 
      break;
  }  
  homekit_characteristic_notify(&cha_target_door_state, cha_target_door_state.value);

}

void loop() {

  // If a change in one of the sensors has been detected, re-run the current state getter
  if ( sensor_interrupt == TRUE ) {
    homekit_value_t new_state = cha_current_door_state_getter();
    homekit_characteristic_notify(&cha_current_door_state, new_state);
    sensor_interrupt = FALSE;
  }
  

  my_homekit_loop();
  delay(10);
}
