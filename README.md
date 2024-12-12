
  RF Power Meter Using HP33330B Probe and CYD
(Cheap Yellow Display) ESP32-2432S028 resistive 

				by F1CJN, F1GE and F1BHY

		alain.fort.f1cjn at gmail.com December 2024








![CYD_Powermeter](https://github.com/user-attachments/assets/ba34e3ca-1f38-4ccf-a4b0-c056c3f7e644)












This digital wattmeter uses a Hewlett Packard HP33330B diode detector probe. This probe has a negative voltage output.  
The advantage of using this probe is that the measured level is very accurate and almost independent of the input frequency up to 22 GHz, which is not the case with commercial integrated circuits. The probe is given an accuracy of +-0.3dB up to 12.5GHz and +- 0.6dB beyond.
This probe was available from Marcel F1GE    f1ge.mg@gmail.com

Any detector diode with negative output voltage can be suitable for the assembly, after calibration.

The assembly includes a very low offset operational amplifier mounted as a -2 gain amplifier. The output voltage of the amplifier is between 0 and 4 volts, which allows an accurate measurement in the range between -30dBm and +20 dBm, a range which can be extended with an input attenuator.

The voltage measurement is performed by a Texas Instruments ADS1115 A/D converter that allows 32768 different measurement values in the range 0 to 4.095 Volt. The least significant value is of the order of 0.150 mV.

The information processing and display are done by a CYD (Cheap Yellow Display) that can be found on Aliexpress for about fifteen Euros. Order the CYD model with ESP32-2432S028 type R resistive touchpad (with resistive touchscreen exclusively). However, there are two models with this same reference that have screens with inverted colors. To invert the colors, touch the screen in the display area. This mode is then memorized.

5V power connection for the CYD
The 5V supply of the CYD is taken from the PCB connector and sent to the CYD through a SS14 Schottky diode or equivalent. The cathode of the diode is connected to the 5V track of the CYD micro-USB connector.

Important: Do not connect the red wire (3.3V) coming from the 4-pin connector of the CYD.

Please note: Do not rely on the colors of the wires in the CYD socket as they may vary depending on delivery.


 

Functioning
When powered on, if the electronic assembly is not powered or absent, the card displays “ADS1115 “not connected”.

Left button: It allows to compensate the reading when an attenuator is mounted upstream of the probe (from 0 to 40 dB) which allows at most to read 100 mW full scale with 0 dB of attenuation and 1kW (60dBm) full scale with 40dB of attenuation. The value of the attenuation is displayed on the quadrant.

Central button: allows you to select one diode from 3. The selected diode is memorized and selected the next time you switch on.

Right button: It allows you to read the voltage measured at the output of the OPA192 amplifier, which is twice the actual voltage at the probe terminals. The voltage is approximately 0 mV without an input signal and approximately 4096 at full scale.
A second press of the key returns to dBm/mW mode.

Software: The program is compiled with the Arduino IDE V2. Check when using that the libraries have been installed, in particular tft_eSPI and ADS1115_WE.

For Arduino novices, follow the procedure described below.
Using the Arduino IDE (Integrated Development Environment) from V.2

In tools, select the “Wroom ESP32-Wroom-DA-Module” card and
load the libraries if you have never installed them:
- ADS1115_WE
- EEPROM                                                                                                              
- XPT2046_Bitbang_slim (Important to use “_slim”
- tft_eSPI to be retrieved from the attached zip file and copied into Arduino/libraries
(Above all, do not install this library from the Arduino IDE).
Select in the tools menu the COM port using the CYD module.

Note: The program was calibrated with an HP 8648C generator and at a frequency of 100 MHz using a representative probe.
Between -30dBm and -10 dBm the calculation (quadratic interpolation) is performed from the values in the HP documentation.                                                                                     
Between -10dBm and +15dbm 'linear' zone the calculation is carried out by linear interpolation from point measurements every 5dB.
The voltage is measured at the output of the operational amplifier, available by pressing the right button.
If you calibrate your diode yourself, the values should be entered starting from lines 372.

On Youtube
https://www.youtube.com/watch?v=b4qHZAA8FIw
 
