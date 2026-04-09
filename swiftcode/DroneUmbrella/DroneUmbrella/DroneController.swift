//
//  DroneController.swift
//  DroneUmbrella
//
//  Created by Jet Trommer on 4/9/26.
//

import Foundation
import CoreMotion
import CoreLocation
import Network
import Combine

// Exact memory match to the ESP32 struct (7 * 4 bytes = 28 bytes)
struct DroneData {
    var pitch: Float32 = 0
    var roll: Float32 = 0
    var enabled: Int32 = 0  // 1 = Arm, 0 = Disarm
}

class DroneController: NSObject, CLLocationManagerDelegate, ObservableObject {
    @Published var isArmed: Bool = false
    @Published var tracking: Bool = false
    @Published var speed: Double = 0
    
//    private var motionManager = CMMotionManager()
    private let udpConnection: NWConnection
    private let locationManager = CLLocationManager()
    
    private var timer: Timer?
    
    override init() {
        let host = NWEndpoint.Host("192.168.4.1")
        let port = NWEndpoint.Port(integerLiteral: 4444)
        
        udpConnection = NWConnection(host: host, port: port, using: .udp)
        super.init()
        udpConnection.start(queue: .global())

        startSensors()
    }
    
    private func startSensors() {
        locationManager.delegate = self
        locationManager.requestWhenInUseAuthorization()
        locationManager.startUpdatingLocation()
        print(locationManager.authorizationStatus)
        
        print("requested")
        
//        guard motionManager.isDeviceMotionAvailable else { return }
//        
//        // Use Apple's sensor fusion (requires compass + gyro + accel)
//        motionManager.deviceMotionUpdateInterval = 1.0 / 50.0 // 50 Hz
//        motionManager.startDeviceMotionUpdates(using: .xMagneticNorthZVertical)
        
//        // Run network transmission on a 50Hz timer loop
//        timer = Timer.scheduledTimer(withTimeInterval: 0.02, repeats: true) { [weak self] _ in
//            self?.sendPacket()
//        }
        
    }

    // Handler required to conform to CLLocationManagerDelegate
    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        print("updating")
        guard let loc = locations.last else { return }
        
        let speed = loc.speed
        let heading = loc.course * .pi / 180.0
        
        self.speed = speed
        
        if speed < 0 || speed > 1 { return }
        
        let data = DroneData(pitch: Float32(speed * sin(heading)), roll: Float32(speed * cos(heading)), enabled: 1)
        
        sendData(data)
    }
    
    func sendData(_ data: DroneData) {
        var packet = data
        
        withUnsafeBytes(of: &packet) { rawBytes in
            let payload = Data(rawBytes)
            udpConnection.send(content: payload, completion: .idempotent)
        }
    }
}
