# Entropon Decelerator

This Arduino Nano 33 IoT project lies at the heart of the [Entroponics](https://entroponics.com) exhibit making its debut at the [Holistic Quantum Activation Art Expo](https://www.instagram.com/holisticquantumactivationart/), a parody new-age art show in Philadelphia on 2024-09-20.

The exhibit consists of a “deceleration chamber” that purports to allow a visitor to experience a localized slowdown of time. To visualize this, this Arduino project drives two pairs of clocks showing the current time inside and outside the chamber. With the press of a button, the “inside” clocks begin to lose time compared to the “outside” ones. Further button presses stabilize and reset the clocks.

The plan is to support both digital (6-digit 7-segment displays via I2C HT16K33 drivers) and analog (via Lavet stepper motors and H-bridges) clock styles.

Another planned feature is the ability to send a request to a BOCA thermal ticket printer to print a certificate of time saved in the deceleration chamber.