# SD Card Testing

- Four Main Tests
  - CardInfo: View SD Card Info such as: size, files, format type, etc...
  - Files: Create And Delete Files From SD Card.
  - DumpFile: Read all the data from a file and send it throw serial.
  - ReadWrite: Read From And Write To a file in the SD Card.

#### Circuit connections are the same across all tests which are:

- CS (Chip Select): to D10 on Arduino.
- MISO (Master In Slave Out): to D12 on Arduino.
- CLK (Clock): to D13 on Arduino
- MOSI (Master Out Slave In): to D11 on Arduino
