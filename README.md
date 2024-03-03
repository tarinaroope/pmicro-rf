# pmicro-rf

This repository hosts a simple RF transmitter and receiver library designed for use with cheap ASK/OOK modulation transmitters and receivers, such as the Hope RFM family. The library offers a straightforward implementation that can be easily ported to various popular microcontrollers.

The protocol used in this library is very similar to the one used by RadioHead's RH_ASK driver.

## Features
- Supports ASK/OOK modulation transmitters and receivers, including the Hope RFM family.
- Simple implementation for easy portability to most popular microcontrollers.
- Ready implementations for Raspberry Pi Pico (both transmitter and receiver) and Arduino (ATtiny85) transmitter.
- Operates at least at rate of 1000 b/s.
- Supports dynamic transmission rate recognition at the receiver side.
- Capable of sending messages up to 64 bits in length.
- Protocol supports CRC, although not yet implemented.
- No error correction at present, but may be added in future updates.

## Background
This library was originally developed to provide a simple 433MHz RF implementation for personal use with temperature, humidity, and CO2 sensors at home. It aims to offer a lightweight solution for transmitting and receiving data over RF channels.

## Contributing
Contributions are welcome.