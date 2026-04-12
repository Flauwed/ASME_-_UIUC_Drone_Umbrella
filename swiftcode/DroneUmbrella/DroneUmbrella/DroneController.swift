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
import MultipeerConnectivity

class DroneController: NSObject, ObservableObject {
    public static let serviceType = "drone"
    
    @Published var paired: Bool = false
    @Published var pairedID: MCPeerID?
    @Published var availablePeers: [MCPeerID] = []
    
    private var currentLocation: CLLocation?
        
    @Published var nsDiff: Double = 0
    @Published var ewDiff: Double = 0
    
    private func recievedLoc(peerLocation: CLLocation) {
        guard let currentLocation = currentLocation else { return }
        
        // Fix longitude
        let northRef = CLLocation(latitude: peerLocation.coordinate.latitude, longitude: currentLocation.coordinate.longitude)
        nsDiff = northRef.distance(from: currentLocation)
        if (currentLocation.coordinate.latitude > peerLocation.coordinate.latitude) {
            nsDiff *= -1
        }
        
        // Fix latitude
        let eastRef = CLLocation(latitude: currentLocation.coordinate.latitude, longitude: peerLocation.coordinate.longitude)
        var ewDiff = eastRef.distance(from: currentLocation)
        if (currentLocation.coordinate.longitude > peerLocation.coordinate.longitude) {
            ewDiff *= -1
        }
    }
        
    private let locationManager = CLLocationManager()
    private var motionManager = CMMotionManager()
    
    private var myPeerID: MCPeerID
    
    private let serviceAdvertiser: MCNearbyServiceAdvertiser
    public let serviceBrowser: MCNearbyServiceBrowser
    public let session: MCSession
    
    init(username: String) {
        myPeerID = MCPeerID(displayName: username)
        
        session = MCSession(peer: myPeerID, securityIdentity: nil, encryptionPreference: .none)
        serviceAdvertiser = MCNearbyServiceAdvertiser(peer: myPeerID, discoveryInfo: nil, serviceType: DroneController.serviceType)
        serviceBrowser = MCNearbyServiceBrowser(peer: myPeerID, serviceType: DroneController.serviceType)
                
        super.init()
        startSensors()
        startMultipeer()
    }
    
    // TODO: Add Deinit
    
    private func startMultipeer() {
        session.delegate = self
        serviceAdvertiser.delegate = self
        serviceBrowser.delegate = self
                        
        serviceAdvertiser.startAdvertisingPeer()
        serviceBrowser.startBrowsingForPeers()
    }
    
    private func startSensors() {
        locationManager.delegate = self
        locationManager.requestWhenInUseAuthorization()
        locationManager.startUpdatingLocation()
        
        if motionManager.isAccelerometerAvailable {
            motionManager.deviceMotionUpdateInterval = 1 / 50.0
            motionManager.startDeviceMotionUpdates(using: .xTrueNorthZVertical, to: .main) { motion, error in
                guard let _ = motion else { return }
                
                // TODO: update pitch and roll with acceleration
            }
        }
        
    }
}

extension DroneController: CLLocationManagerDelegate {
    // Handler required to conform to CLLocationManagerDelegate
    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let loc = locations.last else { return }
        
        let speed = loc.speed
        let heading = loc.course * .pi / 180.0
                
        if speed < 0 || speed > 1 { return }
        
        let _ = Float32(speed * cos(heading))
        let _ = Float32(speed * sin(heading))
        
        print("updating location")
        currentLocation = loc
        if (!session.connectedPeers.isEmpty) {
            
            do {
                var data = Data()
                var lat = loc.coordinate.latitude
                var lon = loc.coordinate.latitude
                data.append(Data(bytes: &lat, count: 8))
                data.append(Data(bytes: &lon, count: 8))
                
                try session.send(data, toPeers: session.connectedPeers, with: .unreliable)
            } catch { }
        }
    }
}

extension DroneController: MCSessionDelegate {
    
    // Peer changes state
    func session(_ session: MCSession, peer peerID: MCPeerID, didChange state: MCSessionState) {
        switch state {
        case .connected:
            // Make changes on the main thread since views are listening for changes
            DispatchQueue.main.async {
                self.paired = true
                self.pairedID = peerID
            }
        break
        case .notConnected:
            DispatchQueue.main.async {
                self.paired = false
                self.pairedID = nil
            }
        break
        default:
            DispatchQueue.main.async {
                self.paired = false
            }
        }
    }
    
    func session(_ session: MCSession, didReceive data: Data, fromPeer peerID: MCPeerID) {
        guard data.count == 16 else { return }
        
        let lat = data.withUnsafeBytes { $0.load(fromByteOffset: 0, as: Double.self) }
        let lon = data.withUnsafeBytes { $0.load(fromByteOffset: 8, as: Double.self) }

        recievedLoc(peerLocation: CLLocation(latitude: lat, longitude: lon))
    }
    
    func session(_ session: MCSession, didReceive stream: InputStream, withName streamName: String, fromPeer peerID: MCPeerID) { }
    
    func session(_ session: MCSession, didStartReceivingResourceWithName resourceName: String, fromPeer peerID: MCPeerID, with progress: Progress) { }
    
    func session(_ session: MCSession, didFinishReceivingResourceWithName resourceName: String, fromPeer peerID: MCPeerID, at localURL: URL?, withError error: (any Error)?) { }
}

extension DroneController: MCNearbyServiceAdvertiserDelegate {
    // For some reason can't start advertising
    func advertiser(_ advertiser: MCNearbyServiceAdvertiser, didNotStartAdvertisingPeer error: Error) {
        print("ServiceAdvertiser didNotStartAdvertisingPeer: \(String(describing: error))")
    }
    
    // Recieved invitation
    func advertiser(_ advertiser: MCNearbyServiceAdvertiser, didReceiveInvitationFromPeer peerID: MCPeerID, withContext context: Data?, invitationHandler: @escaping (Bool, MCSession?) -> Void) {
        invitationHandler(true, session) // Auto accept
    }
}

extension DroneController: MCNearbyServiceBrowserDelegate {
    // For some reason can't start browsing
    func browser(_ browser: MCNearbyServiceBrowser, didNotStartBrowsingForPeers error: Error) {
        print("ServiceBroser didNotStartBrowsingForPeers: \(String(describing: error))")
    }
    
    // Found nearby peer (advertising our serviceType)
    func browser(_ browser: MCNearbyServiceBrowser, foundPeer peerID: MCPeerID, withDiscoveryInfo info: [String : String]?) {
        DispatchQueue.main.async {
            self.availablePeers.append(peerID)
            print("Found peer: \(peerID.displayName)")
        }
    }
    
    // Lost nearby peer
    func browser(_ browser: MCNearbyServiceBrowser, lostPeer peerID: MCPeerID) {
        DispatchQueue.main.async {
            DispatchQueue.main.async {
                self.availablePeers.removeAll { $0 == peerID }
            }
        }
    }
}
