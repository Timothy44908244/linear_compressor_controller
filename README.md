# linear_compressor_controller
see Read Me for instructions

●	Current software version 2.4.4 contains all the basic functions and extra feature collision detection with full comments. 

●	Req value 1~254 correspond to PWM duty cycle 30%~80%.

●	At 255 req value, duty cycle will be 90% and short circuit protection will be temporally removed. Decrease the req value to turn the short circuit protection back on. 

●	The collision detection is still in test stage. It is best tested with around 170 “req” value (i.e. around 55% PWM duty cycle).

●	 If collision detected, shaft will be stopped and the collision detection will be turned off automatically.

●	When the collision detection is on, no JSON package can be received.

●	LED1: warning, LED2: receiving/transmitting JSON package, LED3: collision protection.

************************************************************

To test the collision detection, please follow these steps:
1.	Send JSON package to make the shaft running at req value close to 170.
2.	Send single byte ‘#’ to turn this feature on.
3.	Test head collision.
4.	If success, the shaft will be stopped after collision. LED1 on, LED3 off.
5.	To restart the shaft, send another JSON package to operate normally.
6.	To perform another collision test, send ‘#’ to turn it back on.
7.	To change the mass flow rate while the feature is turned on, send a second ‘#’ to toggle the flag and turn the feature off, then send JSON package to change flow rate.


To force trigger a short circuit protection interrupt, send a single ‘$’ byte. Driving signals will be immediately cut off.

To test mass flow control, please follow the examples of possible JSON package inputs written in the “json_example” text file under project directory.

If a corrupted JSON package is sent (with different number of left curly brackets and right curly brackets), send a single ‘|’ byte to force exit the “JSON receiving state” and go back to “normal receiving state”.
