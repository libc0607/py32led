## PY32LED TX Demo  
A very simple demo of TX side.  

### Commands  

Note that the demo protocol implemented now only contains a 5-bit address and 5-bit for each color.  
It's better to use a customized protocol to carry more informations.  

---
```
tx_frame(address, r, g, b);
```
Set all LEDs' at \<address\> color to (r, g, b).  

---
```
tx_addr(current_addr, new_addr);
```
Set all LEDs at \<current_addr\> a new address \<new_addr\>.   
Since flash programming requires a significantly large (compared with normal) current, it's better to place the receiver inductor as close to the transmitter coil as you can.  

