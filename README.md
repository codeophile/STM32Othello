# STM32 Othello
Play Othello (also known as Reversi) against the computer with selectable levels of difficulty: "Easy" and "Challenging."

## Features

- Easy to build - not many parts
- Uses inexpensive STM32L412 processor.
- You can build my board (I used JLCPCB) or to avoid soldering surface mount, you can breadboard the whole thing using a NUCLEO-L412KB board ($10.99 at Digikey).
- Uses inexpensive ($3.00 on Aliexpress) 8x8 matrix of WS2812B smart LEDs. 
- Playing Othello is very easy to learn as the rules are super simple. Playing well requires strategy.

## Files

- Hardware folder contains schematic, gerbers, and the Eagle project file. Be aware that Eagle project files were created in Eagle 5.x. I'm not sure if it will open OK in newer versions (it should!).
- Software folder contains all the c code and a README for getting started. 
