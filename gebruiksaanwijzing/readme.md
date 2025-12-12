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
debug [on/off]  : Toon 1 maal de debug waardes  
start : Start de linefollower
stop : Stopt de linefollower  
set cycle [µs] : Stelt de cyclus tijd in
set power [0..255] : Stelt de power in (255 => 8.4V)
set diff [0..1] : Stelt de diff in  
set kp [0..] : Stelt de kp in  
set ki [0..] : Stelt de ki in  
set kd [0..] : Stelt de kd in  
calibrate black : Calibreer de zwarte waardes  
calibrate white : Calibreer de witte waardes  

### kalibratie
Zet de linefollower op de zwarte lijn en gebruik het calibrate black command. Deze maakt meerdere metingen en pakt het gemiddelde hiervan.  
Zet de linefollower op de witte achtergrond en gebruik het calibrate white command.  

### settings
De robot rijdt stabiel met volgende parameters: 
Kp : 9.00  
Ki : 0.00  
Kd : 0.14  
Diff : 0.55  
Power : 120  


### start/stop button
uitleg locatie + werking start/stop button
Knop bevindt zich op de pcb. Dit is een toggle interrupt laag actieve knop.  
Deze zet een flag aan, die in de loop dezelfde variabele run toggelt.

