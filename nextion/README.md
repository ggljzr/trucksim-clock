# Project for Nextion display

* Display used: Nextion Enhanced NX4832K035 3.5" 480 x 320
* IDE: [Nextion Editor 1.65.1](https://nextion.tech/nextion-editor/#_section1)
* Configured baudrate: 115200

## Command documentation

You can find documentation for Nextion commands [here](https://nextion.tech/instruction-set/).

## Pages

* ``page 0`` -- splash screen, displayed before the game SDK is loaded.
* ``page 1`` -- main page.

## Main page text labels

Names and descriptions for the labels used on main page. You can use either object ``id`` or 
``objname`` to set the text of the label via serial interface.

| id | objname  | description            | format example      |
|----|----------|------------------------|---------------------|
| 1  | clock    | game time clock        | Mon 13:34           |
| 2  | distance | distance to nav point  | 1234 km             |
| 3  | eta      | ETA to nav point       | Tue 12:58 (23h 24m) |
| 4  | rest     | Time to next rest stop | Mon 23:37 (10h 03m) |

## Example

This is an example on how to set the text of the label via serial port in Python:

```python

from serial import Serial

with Serial("COM4", baudrate=115200) as ser:
    # first set displayed page to 1 (page 0 is splashscreen)
    # all commands are terminated with three 0xFF bytes
    ser.write(b"page 1\xff\xff\xff")

    text = b"Hello!"
    objname = b"clock"
    cmd = objname + b'.txt="' + text + b'"\xff\xff\xff'
    # cmd = b'clock.txt="Hello!"\xff\xff\xff'
    ser.write(cmd)

```
