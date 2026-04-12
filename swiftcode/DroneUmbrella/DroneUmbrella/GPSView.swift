//
//  ContentView.swift
//  DroneUmbrella
//
//  Created by Jet Trommer on 4/9/26.
//

import SwiftUI
import MultipeerConnectivity

struct GPSView: View {
    @StateObject var drone: DroneController
    
    
    var body: some View {
        VStack(spacing: 50) {
            HStack {
                Circle()
                    .fill(drone.paired ? Color.green : Color.red)
                    .frame(width: 12, height: 12)
                Text(drone.paired ? "Paired with " + drone.pairedID!.displayName : "Disconnected")
                    .font(.headline)
                    .foregroundColor(drone.paired ? .green : .red)
            }
            .padding(.top, 20)
            
            // Not paired yet, show list of available peers
            if (!drone.paired) {
                List(drone.availablePeers, id: \.self) { peer in
                    Button(peer.displayName) {
                        drone.serviceBrowser.invitePeer(peer, to: drone.session, withContext: nil, timeout: 30)
                    }
                }
            } else {
                Text("North South Diff to Peer")
                Text(String(drone.nsDiff))
                Text("East West Diff to Peer")
                Text(String(drone.ewDiff))
            }
        }
    }
}
