# PY32LED RX Demo
A very simple demo of RX side.  
 - Mainly copied & pasted from Puya PY32F030 LL Examples. Dirty code warning. 
 - Use 0x80003F80 +0x80 page as config eeprom.
 - Use TIM1 to generate microsecond timestamps.
 - Use TIM3 to generate PWM output.
 - Use COMP1 to demodulate the signal from carrier.
 - Running at 8MHz.

When programming, the option bytes should be tuned manually.  
You should set: nRST as regular GPIO; BOR Level to 1.7V/1.8V.  
As for the programming tool, I recommend using PWLINK2 Lite -- it's selling on taobao for 9.9 CNY, and easy to set option bytes in GUI.
