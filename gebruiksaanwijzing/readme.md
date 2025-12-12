# Gebruiksaanwijzing

### opladen / vervangen batterijen
Koppel de batterij los van de linefollower.  
Gebruik een balanceerlader om hem via de balansconnector op te laden.  
Koppel los na het opladen en laat de batterij rusten.  
Koppel de batterij terug aan de linefollower.  

### draadloze communicatie
#### verbinding maken
Gebruik de Serial Bluetooth Terminal app.  
Ga naar devices=>Bluetooth BLE=>Scan selecteer het juiste device.  
Keer terug en klik naast het vuilbakje op connect.  

#### commando's
debug [on/off]  
start  
stop  
set cycle [µs]  
set power [0..255]  
set diff [0..1]  
set kp [0..]  
set ki [0..]  
set kd [0..]  
calibrate black  
calibrate white  

### kalibratie
Zet de linefollower op de zwarte lijn en gebruik het calibrate black command. Deze maakt meerdere metingen en pakt het gemiddelde hiervan.  
Zet de linefollower op de witte achtergrond en gebruik het calibrate white command.  

### settings
De robot rijdt stabiel met volgende parameters: 
Kp : x  
Ki : x  
Kd : x  
Diff : x  
Power : x  


### start/stop button
uitleg locatie + werking start/stop button
Knop bevindt zich op de pcb. Dit is een toggle interrupt laag actieve knop.  
Deze zet een flag aan, die in de loop dezelfde variabele run toggelt.

