# DMX512A Stage Lighting Controller with TM4C123GH6PM

## Project Overview
This project implements a **DMX512A stage lighting controller** using the **TI TM4C123GH6PM microcontroller** and the **RS-485 communication protocol**. The system provides reliable control over lighting and movement effects in stage environments. It supports **precise LED control** using **Pulse Width Modulation (PWM)**. Two operational modes, **Controller** and **Device**, enable the microcontroller to send or receive DMX512A data as required.

## Key Features
- **DMX512A Protocol**: A reliable standard for controlling lighting and other stage equipment.
- **RS-485 Communication**: Ensures robust data transmission over the DMX512A bus.
- **Efficient PCB Design**: A PCB with surface-mount components was designed, reducing hardware assembly time by 20%.
- **UART Communication**: Used to transmit and receive data between the microcontroller and the PCB.
- **PWM Control**: Provides precise control of lighting intensity and motor movements with a latency of less than 10ms.
- **EEPROM for Mode Storage**: The system supports two modes:
  - **Controller Mode**: Sends data to control lighting.
  - **Device Mode**: Receives data from another controller.
- **Command-Line Interface**: Developed using C programming, it allows real-time interaction and monitoring via **PuTTY**.

## Hardware Components
1. **TI TM4C123GH6PM Microcontroller**: Core processing unit for lighting control.
2. **RS-485 Transceiver**: Facilitates robust communication over the DMX512A bus.
3. **LEDs**: Used for lighting.
4. **Printed Circuit Board (PCB)**: Includes surface-mount components for efficient assembly and reliable signal transmission.

## Software Components
- **Embedded C**: Used for programming the microcontroller.
- **UART**: For transmitting and receiving data between the microcontroller and the RS-485 IC.
- **PWM**: For controlling the lighting intensity and motor movements.
- **EEPROM**: Stores operational mode information (Controller or Device).
- **PuTTY**: Command-line interface for real-time interaction with the microcontroller.

## How It Works
1. **System Setup**:
   - The microcontroller sends data to the RS-485 IC via UART for communication over the DMX512A bus.
   - The system operates in either **Controller Mode** (sends data) or **Device Mode** (receives data), which is stored and fetched from the EEPROM.

2. **Lighting**:
   - **PWM** is used to precisely control the intensity of the LEDs with a response time of less than 10ms.

3. **User Interaction**:
   - A **command-line interface** is provided via **PuTTY**, enabling users to send commands and view real-time data from the microcontroller.

## Installation and Usage
### Prerequisites
- **TI TM4C123GH6PM LaunchPad** or compatible hardware.
- **UART and RS-485 IC** for communication.
- **PuTTY** (or any terminal interface) for real-time control.

### Steps to Run:
1. Clone or download the project source code.
2. Flash the provided code onto the TM4C123GH6PM microcontroller using a compatible IDE (such as **Code Composer Studio**).
3. Connect the DMX512A bus and ensure that all hardware components are properly connected (LEDs, servo motors, etc.).
4. Open **PuTTY** or any terminal software and connect to the microcontroller using UART.
5. Use the command-line interface to send commands and control the system in real-time.

### Example Commands:
- **Controller Mode**: Allows the microcontroller to send data to control lights.
- **Device Mode**: Receives data from another controller on the DMX512A bus.

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Contact
For questions or further inquiries, contact the project creator at [mvsrsarma30@gmail.com](mailto:mvsrsarma30@gmail.com).

---

This **README** provides a structured overview of the project, including its purpose, features, hardware, and software components, along with installation and usage instructions. It ensures that the project is easy to understand and implement for future users or collaborators.
