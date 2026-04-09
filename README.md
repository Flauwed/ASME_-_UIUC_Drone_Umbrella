# FLYING DRONE UMBRELLA
We are here to build a drone that is an umbrella that drones and umbrellas.

Flies around above you and shields you from the rain like a valiant knight of sacred purpose. Engineering open house is not ready for the beauty of our creation.

## File Directory
`receiver_iphone/receiver_iphone.ino` handles receiving data from our Swift application and transmitting it to the flight controller. It does this by setting up a temporary WiFi network that the user's phone can connect to. This is currently in use.

`swiftcode/DroneUmbrella/DroneController.swift` connects to the receiver ESP32's network to send data constructed by processing various iPhone sensors. This is currently in use.

`swifcode/DroneUmbrella/ContentView.swift` manages the UI elements for the drone controller application. This is currently in use.

`receiver/receiver.ino` **(NOT IN USE)** handles receiving data from the sender ESP32 and transmitting it to the flight controller.

`sender/sender.ino` **(NOT IN USE)** processes MPU data into usable yaw, pitch, roll, and acceleration figures before sending them to the receiever ESP32.