# ADS1115_IDF

### ADS1115 library for the espressif IDF environment. 

* Conversion state can be checked.

* Library handles the I2C communication (however must be pre-configured) *no semaphores are used, so these will need to be added manually if needed.

* Most functions return esp_err_t for logging and error detection.

! See main.c for a basic example (and others when/if added) for implimentation and header file for notable comments.

! Please request changes if needed.
