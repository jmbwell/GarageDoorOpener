/*
 * my_accessory.c
 * Define the accessory in C language using the Macro in characteristics.h
 *
 *  Created on: 2020-06-27
 *      Author: jmbwell (John Burwell)
 */

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

void my_accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
}

// Garage Door Opener (HAP section 8.16)
// Required Characteristics:
// - CURRENT_DOOR_STATE
// - TARGET_DOOR_STATE
// - OBSTRUCTION_DETECTED

// Optional Characteristics:
// - NAME
// - LOCK_CURRENT_STATE
// - LOCK_TARGET_STATE

// format: uint8; HAP section 9.30; 0 = opened, 1 = closed, 2 = opening, 3 = closing, 4 = stopped not open or closed
homekit_characteristic_t cha_current_door_state = HOMEKIT_CHARACTERISTIC_(CURRENT_DOOR_STATE, 4);

// format: uint8; HAP section 9.118; 0 = opened, 1 = closed
homekit_characteristic_t cha_target_door_state = HOMEKIT_CHARACTERISTIC_(TARGET_DOOR_STATE, NULL);

// format: bool; HAP section 9.65; 0 = no obstruction, 1 = obstruction detected
homekit_characteristic_t cha_obstruction_detected = HOMEKIT_CHARACTERISTIC_(OBSTRUCTION_DETECTED, 0);

// format: string; HAP section 9.62; max length 64
homekit_characteristic_t cha_name = HOMEKIT_CHARACTERISTIC_(NAME, "GarageDoorOpener-01");

// format: uint8; HAP section 9.30; 0 = unsecured, 1 = secured, 2 = jammed, 3 = unknown
homekit_characteristic_t cha_lock_current_state = HOMEKIT_CHARACTERISTIC_(LOCK_CURRENT_STATE, 3);

// format: uint8; HAP section 9.30; 0 = unsecured, 1 = secured
homekit_characteristic_t cha_lock_target_state = HOMEKIT_CHARACTERISTIC_(LOCK_TARGET_STATE, NULL);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_garage, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "GarageDoorOpener-01"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266/ESP32"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
		HOMEKIT_SERVICE(GARAGE_DOOR_OPENER, .primary=true, .characteristics=(homekit_characteristic_t*[]){
			&cha_current_door_state,
			&cha_target_door_state,
			&cha_obstruction_detected,
			&cha_name,
			&cha_lock_current_state, 
			&cha_lock_target_state, 
			NULL
		}),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
		.accessories = accessories,
		.password = "111-11-111"
};
