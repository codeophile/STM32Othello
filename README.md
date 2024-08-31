# STM32 Othello
Plays a decent game of Othello (also known as Reversi) against a human player.

## Features

- Easy to build - not many parts
- Uses inexpensive STM32L412 processor.
- You can build my board (I used JLCPCB) or to avoid soldering surface mount you can breadboard the whole thing using a NUCLEO-L412KB board ($10.99 at Digikey).
- Uses inexpensive ($3.00 on Aliexpress) 8x8 matrix of WS2812B smart LEDs. 
- The game is very quick and easy to learn to play with two levels of difficulty).

## Files

- Hardware folder contains schematic, gerbers, and the Eagle project file. Be aware that project files were created in Eagle 5.x. I'm not sure if it will open okay in newer versions.
- Software folder contains all the c code and a README for getting started. 
