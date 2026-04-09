//
//  DroneController.swift
//  DroneUmbrella
//
//  Created by Jet Trommer on 4/9/26.
//

import Foundation
import CoreMotion
import Network
import Combine

// Exact memory match to the ESP32 struct (7 * 4 bytes = 28 bytes)
struct DroneData {
    var yaw: Float32 = 0
    var pitch: Float32 = 0
    var roll: Float32 = 0
    var ax: Float32 = 0
    var ay: Float32 = 0
    var az: Float32 = 0    // We'll map a UI slider to this for Throttle
    var button: Int32 = 0  // 1 = Arm, 0 = Disarm
}

class DroneController: ObservableObject {
    @Published var isArmed: Bool = false
    @Published var throttle: Float32 = 0.0 // 0.0 to 1.0 UI Slider
    
    private var motionManager = CMMotionManager()
    private var udpConnection: NWConnection?
    private var timer: Timer?
    
    init() {
        setupNetwork()
        startSensors()
    }
    
    private func setupNetwork() {
        // Connect to the ESP32's default SoftAP IP Address
        let host = NWEndpoint.Host("192.168.4.1")
        let port = NWEndpoint.Port(integerLiteral: 4444)
        
        udpConnection = NWConnection(host: host, port: port, using: .udp)
        udpConnection?.stateUpdateHandler = { state in
            print("UDP State: \(state)")
        }
        udpConnection?.start(queue: .global())
    }
    
    private func startSensors() {
        guard motionManager.isDeviceMotionAvailable else { return }
        
        // Use Apple's sensor fusion (requires compass + gyro + accel)
        motionManager.deviceMotionUpdateInterval = 1.0 / 50.0 // 50 Hz
        motionManager.startDeviceMotionUpdates(using: .xMagneticNorthZVertical)
        
        // Run network transmission on a 50Hz timer loop
        timer = Timer.scheduledTimer(withTimeInterval: 0.02, repeats: true) { [weak self] _ in
            self?.sendPacket()
        }
    }
    
    private func sendPacket() {
        guard let data = motionManager.deviceMotion else { return }
        
        // 1. Get perfect Euler angles.
        // We divide by (Double.pi / 4) which is 45 degrees.
        // Tilting the phone 45 degrees yields a float of 1.0 or -1.0.
        let maxAngle = Float32(Double.pi / 4)
        
        var packet = DroneData()
        packet.pitch = Float32(data.attitude.pitch) / maxAngle
        packet.roll = Float32(data.attitude.roll) / maxAngle
        packet.yaw = 0.0 // Map this to a virtual thumb-joystick later
        
        // 2. Clamp angles between -1.0 and 1.0 to prevent overflow
        packet.pitch = max(-1.0, min(1.0, packet.pitch))
        packet.roll = max(-1.0, min(1.0, packet.roll))
        
        // 3. Map UI controls
        packet.az = self.throttle
        packet.button = self.isArmed ? 1 : 0
        
        // 4. Convert Struct to raw Bytes and send via UDP
        withUnsafeBytes(of: &packet) { rawBytes in
            let payload = Data(rawBytes)
            self.udpConnection?.send(content: payload, completion: .idempotent)
        }
    }
}
