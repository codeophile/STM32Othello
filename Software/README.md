## Options

- You can just flash the STM32L412 chip with the code file named OthelloL412.elf.
- Or, you can edit and debug it since all the source files are included.

## Editing and debugging the code

This code could be adapted for any IDE, but to use it without having to make changes, you'll need to use STM32CubeIDE which is free from ST.com. If your new to this, check out the excellent 4 part YouTube series on titled "Getting Started with STM32 and Nucleo" (https://www.youtube.com/watch?v=hyZS2p1tW-g).

## Steps

- Open a new STMCube32IDE project using: File/New/STM32 Project from an Existing STM32CubeMX Configuration file (.ioc)
- Copy the files from the Inc folder to your project's Core/Inc
- Copy the files from the Src folder to your project's Core/Src
- Edit the auto-generated Core/Src/main.c:
	- Find comment line containing "USER CODE BEGIN PFP" and insert below it the line:"void APP_Main();"
	- Find comment line containing "USER CODE BEGIN WHILE" and insert below it the line:"APP_Main();"
 - Build the project
