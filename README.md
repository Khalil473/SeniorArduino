# SD Card Testing

Markup : _ Four Main Tests
_ CardInfo: View SD Card Info such as: size, files, format type, etc...
_ Files: Create And Delete Files From SD Card.
_ DumpFile: Read all the data from a file and send it throw serial.
\_ ReadWrite: Read From And Write To a file in the SD Card.

#### Circuit connections are the same across all tests which are:

Markup : * CS (Chip Select): to D10 on Arduino.
*MISO (Master In Slave Out): to D12 on Arduino.
*CLK (Clock): to D13 on Arduino
*MOSI (Master Out Slave In): to D11 on Arduino
