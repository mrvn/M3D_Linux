// Header files
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "printer.h"
#include "gcode.h"


// Definitions

// Chip details
#define CHIP_NAME ATxmega32C4
#define CHIP_PAGE_SIZE 0x80
#define CHIP_NRWW_SIZE 0x20
#define CHIP_NUMBER_OF_PAGES 0x80
#define CHIP_TOTAL_MEMORY CHIP_NUMBER_OF_PAGES * CHIP_PAGE_SIZE * 2

// Fan types
enum fanTypes {HENGLIXIN = 0x01, LISTENER = 0x02, SHENZHEW = 0x03, NONE= 0xFF};

// Crc seed and table
const uint32_t crc32Seed = 0xFFFFFFFF;
const uint32_t crc32Table[] = {0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D};


// Supporting function implementation
Printer::Printer() {

	// Clear file descriptor
	fd = -1;
	
	// Clear status
	status = 0;
	
	// Set bootloader mode
	bootloaderMode = true;
}

Printer::~Printer() {

	// Close file descriptor if open
	if(fd != -1)
		close(fd);
}

bool Printer::connect() {

	// Initialize variables
        termios settings;
        
        // Close file descriptor if open
        if(fd != -1)
        	close(fd);
        
        // Attempt to connect for 2 seconds
        for(uint8_t i = 0; i < 8; i++) {
        
		// Wait 250 milliseconds
		usleep(250000);
		
		// Check if opening device was successful
		if((fd = open("/dev/micro_m3d", O_RDWR | O_NONBLOCK)) != -1) {
	      
			// Set serial protocol to 8n1 with 115200 baud rate
			memset(&settings, 0, sizeof(settings));
			settings.c_iflag = 0;
			settings.c_oflag = 0;
			settings.c_cflag= CS8 | CREAD | CLOCAL;
			settings.c_lflag = 0;
			settings.c_cc[VMIN] = 1;
			settings.c_cc[VTIME] = 5;
			cfsetospeed(&settings, B115200);
			cfsetispeed(&settings, B115200);

			// Apply settings
			tcsetattr(fd, TCSAFLUSH, &settings);
			tcdrain(fd);

			// Return true
			return true;
		}
	}
	
	// Return false
	return false;
}

bool Printer::isFirmwareValid() {

	// Initialize variables
	string response, eepromSerial;
	uint32_t chipCrc = 0, eepromCrc = 0;
	fanTypes eepromFan;
	uint8_t fanOffset;
	float fanScale;
	int32_t *tempPointer;

	// Check if printer is connected and receiving commands
	if(fd != -1 && sendRequestAscii("M115\r\n")) {
	
		// Check if in bootloader mode
		if(receiveResponseAscii()[0] == 'B') {
	
			// Request crc from chip
			sendRequestAscii('C');
			sendRequestAscii('A');
		
			// Get response
			response = receiveResponseAscii();
		
			// Get chip crc
			for(uint8_t i = 0; i < 4; i++) {
				chipCrc <<= 8;
				chipCrc += static_cast<uint8_t>(response[i]);
			}
		
			// Request eeprom
			sendRequestAscii('S');
		
			// Get response
			response = receiveResponseAscii();
		
			// Check if failed to read eeprom
			if(response.back() != '\r')
		
				// Return false
				return false;
		
			// Get eeprom crc
			for(uint8_t i = 0; i < 4; i++) {
				eepromCrc <<= 8;
				eepromCrc += static_cast<uint8_t>(response[i + 4]);
			}
		
			// Check if firmware is corrupt
			if(chipCrc != eepromCrc)
		
				// Return false
				return false;
		
			// Get eeprom serial
			for(uint8_t i = 0; i < 13; i++)
				eepromSerial.push_back(response[i + 0x2EF]);
			
			// Get eeprom fan
			eepromFan = static_cast<fanTypes>(response[0x2AB]);
		
			// Check if fan needs updating
			if(!eepromFan || eepromFan == NONE) {
		
				// Set fan to default
				eepromFan = HENGLIXIN;
			
				// Check if device is newer
				if(stoi(eepromSerial.substr(2, 6)) >= 150602)
			
					// Set fan to newer
					eepromFan = SHENZHEW;
			
				// Set fan offset and scale
				if(eepromFan == HENGLIXIN) {
					fanOffset = 200;
					fanScale = 0.2165354;
				}
				else if(eepromFan == LISTENER) {
					fanOffset = 145;
					fanScale = 0.1666667;
				}
				else {
					fanOffset = 82;
					fanScale = 0.227451;
				}
				tempPointer = reinterpret_cast<int32_t *>(&fanScale);
			
				// Check if saving fan type and fan offset to eeprom failed
				if(!writeToEeprom(0x2AB, eepromFan) || !writeToEeprom(0x2AC, fanOffset))
			
					// Return false
					return false;
			
				// Check if saving fan scale to eeprom failed
				for(uint8_t i = 0; i < 4; i++)
					if(!writeToEeprom(0x2AD + i, *tempPointer >> 8 * i))
				
						// Return false
						return false;
			}
		
			// Check if extruder current needs updating
			if((eepromSerial == "BK15033001100" || eepromSerial == "BK15040201050" || eepromSerial == "BK15040301050" || eepromSerial == "BK15040602050" || eepromSerial == "BK15040801050" || eepromSerial == "BK15040802100" || eepromSerial == "GR15032702100" || eepromSerial == "GR15033101100" || eepromSerial == "GR15040601100" || eepromSerial == "GR15040701100" || eepromSerial == "OR15032701100" || eepromSerial == "SL15032601050") && static_cast<uint8_t>(response[0x2E8]) + (static_cast<uint8_t>(response[0x2E9]) << 8) != 500)
		
				// Check if saving extruder current to eeprom to be 500 failed
				if(!writeToEeprom(0x2E8, static_cast<uint8_t>(500)) || !writeToEeprom(0x2E9, 500 >> 8))
			
					// Return false
					return false;
		}
		// Return true
		return true;
	}
	
	// Return false
	return false;
}

bool Printer::updateFirmware(const char *file) {

	// Initialize variables
	string response, romBuffer, temp;
	uint32_t chipCrc = 0, eepromCrc = 0, position, romVersion = 0, romCrc = 0;
	ifstream romInput;
	uint16_t pagesToWrite;

	// Check if printer is connected and receiving commands
	if(fd != -1 && sendRequestAscii("M115\r\n")) {
	
		// Check if in bootloader mode
		if(receiveResponseAscii()[0] == 'B') {
	
			// Check if opening rom failed
			romInput.open(file, ios::in | ios::binary);
			if(!romInput.good())

				// Return false
				return false;
		
			// Check if rom contains valid header
			if(romInput.get() != 'M' || romInput.get() != '3' || romInput.get() != 'D' || romInput.get() != '\x1A')
		
				// Return false
				return false;
		
			// Request eeprom
			sendRequestAscii('S');
		
			// Get response
			response = receiveResponseAscii();
		
			// Check if failed to read eeprom
			if(response.back() != '\r')
		
				// Return false
				return false;
		
			// Read in rom version
			for(uint8_t i = 0; i < 4; i++) {
		
				// Check if file is invalid
				if(romInput.peek() == EOF)
			
					// Return false
					return false;
			
				// Otherwise
				else {
			
					// Read in each byte
					romVersion <<= 8;
					romVersion += static_cast<uint8_t>(romInput.get());
				}
			}
		
			// Read in rom crc
			for(uint8_t i = 0; i < 4; i++) {
		
				// Check if file is invalid
				if(romInput.peek() == EOF)
			
					// Return false
					return false;
			
				// Otherwise
				else {
			
					// Read in each byte
					romCrc <<= 8;
					romCrc += static_cast<uint8_t>(romInput.get());
				}
			}
			
			// Read in the rest of the rom
			while(romInput.peek() != EOF)
				romBuffer.push_back(romInput.get());
			romInput.close();

			// Request that chip be erased
			sendRequestAscii('E');

			// Check if chip failed to be erased
			do {
				temp = receiveResponseAscii();
			} while(temp.empty());
			if(temp != "\r")

				// Return false
				return false;

			// Send address zero
			sendRequestAscii('A');
			sendRequestAscii('\x00');
			sendRequestAscii('\x00');

			// Check if address wasn't acknowledged
			if(receiveResponseAscii() != "\r")

				// Return false
				return false;

			// Set pages to write
			pagesToWrite = romBuffer.length() / 2 / CHIP_PAGE_SIZE;
			if(romBuffer.length() / 2 % CHIP_PAGE_SIZE != 0)
				pagesToWrite++;

			// Go through all pages to write
			for(uint16_t i = 0; i < pagesToWrite; i++) {

				// Send write to page request
				sendRequestAscii('B');
				sendRequestAscii(CHIP_PAGE_SIZE * 2 >> 8);
				sendRequestAscii(static_cast<char>(CHIP_PAGE_SIZE * 2));

				// Go through all values for the page
				for(int j = 0; j < CHIP_PAGE_SIZE * 2; j++) {

					// Check if data to be written exists
					position = j + CHIP_PAGE_SIZE * i * 2;
					if(position < romBuffer.length())
	
						// Send value
						sendRequestAscii(romBuffer[position + (position % 2 ? -1 : 1)]);
	
					// Otherwise
					else
	
						// Send arbitrary value
						sendRequestAscii('\x23');
				}

				// Check if chip failed to be flashed
				if(receiveResponseAscii() != "\r")

					// Return false
					return false;
			}

			// Send address zero
			sendRequestAscii('A');
			sendRequestAscii('\x00');
			sendRequestAscii('\x00');

			// Check if address wasn't acknowledged
			if(receiveResponseAscii() != "\r")

				// Return false
				return false;

			// Check if section needs to be zeroed out
			if(!response[0x2E6] || chipCrc != eepromCrc)

				// Check if zeroing out section failed
				if(!writeToEeprom(0x08, 0) || !writeToEeprom(0x09, 0) || !writeToEeprom(0x0A, 0) || !writeToEeprom(0x0B, 0))

					// Return false
					return false;

			// Request crc from chip
			sendRequestAscii('C');
			sendRequestAscii('A');

			// Get response
			temp = receiveResponseAscii();

			// Get chip crc
			for(uint8_t i = 0; i < 4; i++) {
				chipCrc <<= 8;
				chipCrc += static_cast<uint8_t>(temp[i]);
			}
			
			// Check if firmware update failed
			if(chipCrc != __builtin_bswap32(romCrc))

				// Return false
				return false;

			// Check if zeroing out sections in eeprom failed
			if(!writeToEeprom(0x2D6, 0) || !writeToEeprom(0x2D7, 0) || !writeToEeprom(0x2D8, 0) || !writeToEeprom(0x2D9, 0) || !writeToEeprom(0x2DA, 0) || !writeToEeprom(0x2DB, 0) || !writeToEeprom(0x2DC, 0) || !writeToEeprom(0x2DD, 0) || !writeToEeprom(0x2DE, 0) || !writeToEeprom(0x2DF, 0) || !writeToEeprom(0x2E0, 0) || !writeToEeprom(0x2E1, 0) || !writeToEeprom(0x2E2, 0) || !writeToEeprom(0x2E3, 0) || !writeToEeprom(0x2E4, 0) || !writeToEeprom(0x2E5, 0))

				// Return false
				return false;

			// Check if updating firmware version in eeprom failed
			for(uint8_t i = 0; i < 4; i++)
				if(!writeToEeprom(i, romVersion >> 8 * i))

					// Return false
					return false;

			// Check if updating firmware crc in eeprom failed
			for(uint8_t i = 0; i < 4; i++)
				if(!writeToEeprom(i + 4, romCrc >> 8 * i))

					// Return false
					return false;
		}
		
		// Return true
		return true;
	}
	
	// Return false
	return false;
}
		
bool Printer::collectInformation() {

	// Initialize variables
	string response;
	char character;
	uint16_t temp;

	// Loop forever
	while(1) {
	
		// Check if device is already in firmware mode
		sendRequestAscii("M115\r\n");
		while(read(fd, &character, 1) == -1);
		if(character == 'e')
		
			// Break
			break;
		
		// Attempt to put device into firmware mode
		sendRequestAscii('Q');
		
		// Return false if failed to reconnect
		if(!connect())
			return false;
	}
	
	// Catch substring errors
	try {
	
		// Get device info
		sendRequestBinary("M115\r\n");
		response = receiveResponse();
	
		// Set firmware and serial number 
		firmwareVersion = response.substr(response.find("FIRMWARE_VERSION:") + 17, response.find(" ", response.find("FIRMWARE_VERSION:")) - response.find("FIRMWARE_VERSION:") - 17);
		serialNumber = response.substr(response.find("SERIAL_NUMBER:") + 14);
		
		// Get bed offsets
		sendRequestBinary("M578");
		response = receiveResponse();
	
		// Set bed offsets
		backRightOffset = stod(response.substr(response.find("BRO:") + 4, response.find(" ", response.find("BRO:")) - response.find("BRO:") - 4));
		backLeftOffset = stod(response.substr(response.find("BLO:") + 4, response.find(" ", response.find("BLO:")) - response.find("BLO:") - 4));
		frontRightOffset = stod(response.substr(response.find("FRO:") + 4, response.find(" ", response.find("FRO:")) - response.find("FRO:") - 4));
		frontLeftOffset = stod(response.substr(response.find("FLO:") + 4, response.find(" ", response.find("FLO:")) - response.find("FLO:") - 4));
		bedHeightOffset = stod(response.substr(response.find("ZO:") + 3));

		// Get backlash
		sendRequestBinary("M572");
		response = receiveResponse();
	
		// Set backlash values
		backlashX = stod(response.substr(response.find("BX:") + 3, response.find(" ", response.find("BX:")) - response.find("BX:") - 3));
		backlashY = stod(response.substr(response.find("BY:") + 3));
	
		// Get bed orientation
		sendRequestBinary("M573");
		response = receiveResponse();
	
		// Set bed orientation
		backRightOrientation = stod(response.substr(response.find("BR:") + 3, response.find(" ", response.find("BR:")) - response.find("BR:") - 3));
		backLeftOrientation = stod(response.substr(response.find("BL:") + 3, response.find(" ", response.find("BL:")) - response.find("BL:") - 3));
		frontLeftOrientation = stod(response.substr(response.find("FL:") + 3, response.find(" ", response.find("FL:")) - response.find("FL:") - 3));
		frontRightOrientation = stod(response.substr(response.find("FR:") + 3));
	
		// Get status
		sendRequestBinary("M117");
		response = receiveResponse();
	
		// Set valid Z and status
		validZ = response.find("VZ:1") != string::npos; 
		status = stoi(response.substr(response.find("S:") + 2));
		
		// Get filament information
		sendRequestBinary("M576");
		response = receiveResponse();
		
		// Set filament location
		temp = stoi(response.substr(response.find("P:") + 2, response.find(" ", response.find("P:")) - response.find("P:") - 2));
		filamentLocation = (temp & 0xC0) == 0x40 ? INTERNAL : EXTERNAL;
		
		// Set filament type
		filamentType = (temp & 0x3F) < 4 ? static_cast<filamentTypes>(temp & 0x3F) : UNKNOWN;
		
		// Set filament color
		temp = stoi(response.substr(response.find("S:") + 2, response.find(" ", response.find("S:")) - response.find("S:") - 2));
		filamentColor = temp <= 0x2C ? static_cast<filamentColors>(temp) : OTHER_COLOR;
		
		// Set filament temperature
		filamentTemperature = stoi(response.substr(response.find("T:") + 2)) + 100;
	}
	
	// Check if an error has occured
	catch(const out_of_range& exception) {
	
		// Return false
		return false;
	}
	
	//return true
	return true;
}
	
bool Printer::sendRequest(const char *data) {

	// Send data based on if in bootloader mode
	return bootloaderMode ? sendRequestAscii(data) : sendRequestBinary(data);
}

bool Printer::sendRequest(const string &data) {

	// Return if data was sent 
	return sendRequest(data.c_str());
}
	
string Printer::receiveResponse() {

	// Send data based on if in bootloader mode
	return bootloaderMode ? receiveResponseAscii() : receiveResponseBinary();
}

bool Printer::sendRequestAscii(char data) {

	// Initialize variables
	bool returnValue;

	// Send data to the device
	tcflush(fd, TCIOFLUSH);
	returnValue = write(fd, &data, 1) != -1;
	tcdrain(fd);
	
	// Return value
	return returnValue;
}

bool Printer::sendRequestAscii(const char *data) {

	// Initialize variables
	bool returnValue;

	// Send data to the device
	tcflush(fd, TCIOFLUSH);
	returnValue = write(fd, data, strlen(data)) != -1;
	tcdrain(fd);
	
	// Return value
	return returnValue;
}

bool Printer::sendRequestBinary(const char *data) {

	// Initialize variables
	bool returnValue;
	Gcode gcode;
	
	// Check if line was successfully parsed
	if(gcode.parseLine(data)) {
	
		// Send binary request to the device
		tcflush(fd, TCIOFLUSH);
		returnValue = write(fd, gcode.getBinary().data(), gcode.getBinary().size()) != -1;
		tcdrain(fd);
		
		// Set bootloader mode and reconnect if necessary
		bootloaderMode = gcode.getValue('M') == "115" && gcode.getValue('S') == "628";
		if(bootloaderMode)
			while(!connect());
		
		// Return value
		return returnValue;
	}
	
	// Return false
	return false;
}

string Printer::receiveResponseAscii() {

	// Initialize variables
	string response;
	char character;
	uint8_t i;
	
	// Wait 200 milliseconds for a response
	for(i = 0; i < 200 && read(fd, &character, 1) == -1; i++)
		usleep(1000);
	
	// Return an empty string if no response is received
	if(i == 200)
		return response;
	
	// Get response
	do {
		response.push_back(character);
		usleep(50);
	} while(read(fd, &character, 1) != -1);
	
	// Return response
	return response;
}

string Printer::receiveResponseBinary() {

	// Initialize variables
	string response;
	char character;
	uint8_t i;
	
	// Wait 200 ms for a response
	for(i = 0; i < 200 && read(fd, &character, 1) == -1; i++)
		usleep(1000);
	
	// Return an empty string if no response is received
	if(i == 200)
		return response;
	
	// Get response
	while(character != '\n') {
		response.push_back(character);
		while(read(fd, &character, 1) == -1);
	}
	
	// Remove newline character from response
	response.pop_back();
	
	// Return response
	return response;
}

bool Printer::writeToEeprom(uint16_t address, const uint8_t *data, uint16_t length) {

	// Send write to eeprom request
	sendRequestAscii('U');
	sendRequestAscii(address >> 8);
	sendRequestAscii(address);
	sendRequestAscii(length >> 8);
	sendRequestAscii(length);
	
	// Send data
	for(uint16_t i = 0; i < length; i++)
		sendRequestAscii(data[i]);
	
	// Return if write was successful
	return receiveResponseAscii() == "\r";
}

bool Printer::writeToEeprom(uint16_t address, uint8_t data) {

	// Return if write was successful
	return writeToEeprom(address, &data, 1);
}

uint32_t Printer::crc32(int32_t offset, const uint8_t *data, int32_t count) {

	// Initialize variables
	uint32_t crc = 0;
	
	// Update crc
	crc ^= crc32Seed;

	// Go through data
	while(--count >= 0)
	
		// Update crc
		crc = crc32Table[(crc ^ data[offset++]) & 0xFF] ^ (crc >> 8);
	
	// Return updated crc
	return crc ^ crc32Seed;
}