


#include <stdio.h>
#include <wiringPi.h> //provides the functions necessary to interface with the Raspberry Pi's GPIO pins.

//A constant signal is defined which specifies the GPIO pin number (18) to which the DHT22 sensor is connected.
static const unsigned short signal = 18;
unsigned short data[5] = {0, 0, 0, 0, 0}; //An array data is initialized to store the data received from the sensor.

//This defines the readData function which will read data from the DHT22 sensor.
short readData()
{
	unsigned short val = 0x00; //bit value being read
	unsigned short signal_length = 0; //count the length of the current HIGH signal
	unsigned short val_counter = 0; // track of how many bits have been read for the current byte.
	unsigned short loop_counter = 0; // counts the number of HIGH signals encountered

	while (1)
	{
		// Count only HIGH signal
		while (digitalRead(signal) == HIGH) 
		{
			signal_length++; ////Increment the length of the HIGH signal.

			// When sending data ends, high signal occur infinite.
			// So we have to end this infinite loop.
			if (signal_length >= 200) //If the signal remains HIGH for too long, it's considered an error.
			{
				return -1; // Exit the function indicating an error.
			}
			//Wait for 1 microsecond before checking the signal again.
			delayMicroseconds(1);
		} // End of the inner while loop that measures the HIGH signal length.

		// If there was a HIGH signal of any duration...
		if (signal_length > 0)
		{
			loop_counter++;	// HIGH signal counting

			// The DHT22 sends a lot of unstable signals.
			// So extended the counting range.
			if (signal_length < 10)  //If the HIGH signal was very short, it's considered noise or an unstable signal.
			{
				// Unstable signal
				val <<= 1;		// 0 bit. Just shift left by one bit, (this essentially adds a 0 bit to 'val').
			}

			else if (signal_length < 30) //If the signal length was between 10 and 30 microseconds, it's a valid 0 bit.
			{
				// 26~28us means 0 bit
				val <<= 1;		// 0 bit. Just shift left by 1 bit
			}

			else if (signal_length < 85)
			{
				// 70us means 1 bit
				// Shift left and input 0x01 using OR operator
				val <<= 1;
				val |= 1; // set the least significant bit to 1.
			}

			else //Any other signal length is considered noise or an unstable signal.
			{
				// Unstable signal
				return -1;
			}

			signal_length = 0;	// Initialize signal length for next signal
			val_counter++;		// Count for 8 bit data, Increment the counter for bits read.

		} //End of the if condition checking for a signal length greater than 0.

		// The first and second signal is DHT22's start signal.
		// So ignore these data.
		if (loop_counter < 3)
		
		//Reset the current value and bit counter.
		{
			val = 0x00;
			val_counter = 0;
		}

		// If we've read 8 bits...
		if (val_counter >= 8)
		{
			// Store the 8-bit value in the data array.
			data[(loop_counter / 8) - 1] = val;

			//Reset the value and bit counter for the next byte.
			val = 0x00;
			val_counter = 0;
		}
	}
}


int main(void)
{
	float humidity;
	float celsius;
	float fahrenheit;
	short checksum; //validate the data.

	// GPIO Initialization
	if (wiringPiSetupGpio() == -1) //Initialize the GPIO pins using the wiringPi library.
	{	
		//If initialization fails, display an error message and exit the program.
		printf("[x_x] GPIO Initialization FAILED.\n");
		return -1;
	}

	for (unsigned char i = 0; i < 10; i++) //Loop 10 times, trying to read data from the sensor up to 10 times.
	{
		pinMode(signal, OUTPUT); //Set the GPIO pin (connected to the sensor) as an OUTPUT.

		// Send a LOW signal to the sensor to start the data transmission.
		digitalWrite(signal, LOW);
		//Wait for 20 milliseconds.
		delay(20);					// Stay LOW for 5~30 milliseconds
		//Set the GPIO pin to INPUT mode to start reading the data from the sensor.
		pinMode(signal, INPUT);		// 'INPUT' equals 'HIGH' level. And signal read mode

		readData();		// Read DHT22 signal

		// The sum is maybe over 8 bit like this: '0001 0101 1010'.
		// Remove the '9 bit' data using AND operator.
		checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
		//Calculate the checksum by summing up the first four bytes of data and taking the lowest 8 bits.

		// If Check-sum data is correct (NOT 0x00), display humidity and temperature
		//If the calculated checksum matches the checksum byte from the sensor and it's not 0...
		if (data[4] == checksum && checksum != 0x00)
		{
			// * 256 is the same thing '<< 8' (shift).
			//Calculate the humidity value.
			humidity = ((data[0] * 256) + data[1]) / 10.0;


			//Calculate the temperature in Celsius using a corrected method.
			// found that with the original code at temperatures > 25.4 degrees celsius
			// the temperature would print 0.0 and increase further from there.
			// Eventually when the actual temperature drops below 25.4 again
			// it would print the temperature as expected.
			// Some research and comparisin with other C implementation suggest a
			// different calculation of celsius.
			//celsius = data[3] / 10.0; //original
			celsius = (((data[2] & 0x7F)*256) + data[3]) / 10.0; //Juergen Wolf-Hofer

			// If 'data[2]' data like 1000 0000, It means minus temperature
			//If the most significant bit of the third data byte is set, the temperature is negative.
			if (data[2] == 0x80)
			{
				celsius *= -1; //Make the temperature value negative.
			}

			fahrenheit = ((celsius * 9) / 5) + 32; //Convert the Celsius temperature to Fahrenheit.

			// Display all data
			printf("TEMP: %6.2f *C (%6.2f *F) | HUMI: %6.2f %\n\n", celsius, fahrenheit, humidity);
			return 0;
		} //End of the valid checksum check.

		else //If the checksum was invalid...
		{
			printf("[x_x] Invalid Data. Try again.\n\n");
		}

		// Initialize data array for next loop
		for (unsigned char i = 0; i < 5; i++)
		{
			data[i] = 0; //Reset each byte of the data array.
		}

		delay(2000);	// Wait for 2 seconds before the next attempt to read data
	}

	return 0;
}
