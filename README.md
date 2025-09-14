# Entropon Decelerator

This Arduino Nano 33 IoT project lies at the heart of the [Entroponics](https://entroponics.com) exhibit making its debut at the [Holistic Quantum Activation Art Expo](https://www.instagram.com/holisticquantumactivationart/), a parody new-age art show in Philadelphia on 2024-09-20.

The exhibit consists of a “deceleration chamber” that purports to allow a visitor to experience a localized slowdown of time. To visualize this, this Arduino project drives two pairs of clocks showing the current time inside and outside the chamber. With the press of a button, the “inside” clocks begin to lose time compared to the “outside” ones. Further button presses stabilize and reset the clocks.

The plan is to support both digital (6-digit 7-segment displays via I2C HT16K33 drivers) and analog (via Lavet stepper motors and H-bridges) clock styles.

Another planned feature is the ability to send a request to a BOCA thermal ticket printer to print a certificate of time saved in the deceleration chamber.

## Digital clocks

Two identical digital clock displays are mounted inside and outside the chamber. Each one (vaguely inspired by BTTF) features sixteen digits: six (H:M:S, in red) for the outer time, six (H:M:S, in green) for the inner time, two (yellow) for difference in seconds (i.e. time saved), and two (yellow) for "power level". Each display is driven by two two HT16K33 I2C modules supporting eight digits each - one drives the outer time and power level, the other the inner time and difference.

## Analog clocks

Three analog clocks are displayed outside the chamber, showing outer time, inner time, and cumulative time saved (across all sessions). These are standard quartz clocks with Lavet-type stepper motors, separated from their original circuity and driven via H-bridges. On power up, the operator must manually adjust the analog clocks to agree with what's shown on the digital clocks; from there the Arduino will send out pulses for even/odd second advances.