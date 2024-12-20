# Embedded System: NTUEE 2024 Fall Final Project  

## Abstract  
In this project, we aim to combine modern game development with embedded system programming to create a unique and engaging gaming experience. Utilizing the SDL2 library in C++, we designed a game featuring smooth graphics, responsive controls, and captivating mechanics.  

On the hardware side, we programmed the B-L475E-IOT01A development board using STM32CubeIDE. The board functions as the game controller, incorporating sensors and buttons to enable interactive and immersive gameplay. This project bridges the realms of software and hardware, showcasing innovative game mechanics such as tilt navigation and sensor-based inputs to provide a fun and intuitive user experience.  

## How to Run the Project  

1. **Install SDL2 Packages**  
   Follow the instructions provided at [Lazy Foo's SDL Tutorials](https://lazyfoo.net/tutorials/SDL/) to download and set up the necessary packages for running the game.  

2. **Program the Development Board**  
   Open STM32CubeIDE, load the provided code, and flash it onto the B-L475E-IOT01A board.  

3. **Run the Python Script**  
   Execute the Python script to receive accelerometer data from the B-L475E-IOT01A board. Once the connection is successfully established, run the C++ game file to start the game.  

## Game Description  
The game is inspired by the classic Google Chrome dinosaur game, with a flying dinosaur character navigating through various obstacles.  

### Key Features:  
- **Obstacles**: Dodge incoming trees and arrows to avoid losing health.  
- **Sheep**: Collect sheep on the map to gain points.  
- **Health Management**: The game ends when the player loses all three lives.  

### Controls:  
- **Start Game**: Push the button on the development board.  
- **Move Character**: Flip the board to navigate the character up or down.  
- **Recover Health**: Press and hold the button for one second, then release it.  
- **Shoot Fire**: Tap the button briefly to shoot fire and destroy obstacles.  

## Authors  
- **Yi-Kai Li**  
  ![GitHub followers](https://img.shields.io/github/followers/danielliyk?label=Follow&style=social)  
  [GitHub Profile](https://github.com/danielliyk)  

- **Chun-Wei Chen**  
  ![GitHub followers](https://img.shields.io/github/followers/ChenWils?label=Follow&style=social)  
  [GitHub Profile](https://github.com/ChenWils)  

- **Po-Wen Huang**  
  ![GitHub followers](https://img.shields.io/github/followers/PioHuang?label=Follow&style=social)  
  [GitHub Profile](https://github.com/PioHuang)  
