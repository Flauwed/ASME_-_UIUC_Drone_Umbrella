//
//  DroneController.swift
//  DroneUmbrella
//
//  Created by Jet Trommer on 4/9/26.
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
    var thrust: Float32 = 0.5
    var enabled: Int32 = 0  // 1 = Arm, 0 = Disarm
    var kill: Int32 = 0
    var angle: Int32 = 0
    var holdAlt: Int32 = 0
}

class DroneController: NSObject, CLLocationManagerDelegate, ObservableObject {
    @Published var droneData = DroneData() {
        didSet {
            sendData(droneData)
            print("sending packet")
        }
    }
    
    // NEW: Expose connection state to SwiftUI
    @Published var isConnectionReady = false
    
    private var motionManager = CMMotionManager()
    private let udpConnection: NWConnection
    private let locationManager = CLLocationManager()
    
    private var timer: Timer?
    
    override init() {

        let host = NWEndpoint.Host("192.168.4.1")
        let port = NWEndpoint.Port(integerLiteral: 4444)

        self.udpConnection = NWConnection(host: host, port: port, using: .udp)
        super.init()
        
        // Monitor connection status
        self.udpConnection.stateUpdateHandler = { [weak self] state in
            DispatchQueue.main.async {
                switch state {
                case .ready:
                    print("✅ Connection ready")
                    self?.isConnectionReady = true
                case .failed(let error):
                    print("❌ Connection failed: \(error)")
                    self?.isConnectionReady = false
                case .waiting(let error):
                    print("⏳ Connection waiting: \(error)")
                    self?.isConnectionReady = false
                default:
                    break
                }
            }
        }
        
        
        
        self.udpConnection.start(queue: .global())
        startSensors()
        
    }
    
    private func startSensors() {
        locationManager.delegate = self
        locationManager.requestWhenInUseAuthorization()
        locationManager.startUpdatingLocation()
        print(locationManager.authorizationStatus)
        
        print("requested")
        
        if motionManager.isAccelerometerAvailable {
            motionManager.deviceMotionUpdateInterval = 1 / 50.0
            motionManager.startDeviceMotionUpdates(using: .xTrueNorthZVertical, to: .main) { motion, error in
                guard let _ = motion else { return }
                
                // TODO: update pitch and roll with acceleration
            }
        }
        
    }

    // Handler required to conform to CLLocationManagerDelegate
    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let loc = locations.last else { return }
        
        let speed = loc.speed
        let heading = loc.course * .pi / 180.0
                
        if speed < 0 || speed > 1 { return }
        
        let _ = Float32(speed * cos(heading))
        let _ = Float32(speed * sin(heading))
        
        // TODO: update pitch and roll with velocity
    }
    
    func sendData(_ data: DroneData) {
        var packet = data
        
        withUnsafeBytes(of: &packet) { rawBytes in
            let payload = Data(rawBytes)
            
            // Use the built-in defaultMessage context instead of creating a custom one.
            // isComplete: false tells iOS "this stream is still open, do not close the socket!"
            self.udpConnection.send(content: payload, completion: .contentProcessed { error in
                if let error = error {
                    print("Send error: \(error)")
                } else {
                    print("Packet sent successfully!")
                }
            })
        }
    }
}
