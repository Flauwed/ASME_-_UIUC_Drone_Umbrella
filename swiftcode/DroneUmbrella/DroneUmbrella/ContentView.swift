//
//  ContentView.swift
//  DroneUmbrella
//
//  Created by Jet Trommer on 4/9/26.
//

import SwiftUI

struct ContentView: View {
    @StateObject var drone = DroneController()
    
    var enabledBool: Bool {
        drone.droneData.enabled != 0
    }
    
    var body: some View {
        VStack(spacing: 50) {
            
            // ---- NEW: Status Indicator ----
            HStack {
                Circle()
                    .fill(drone.isConnectionReady ? Color.green : Color.red)
                    .frame(width: 12, height: 12)
                Text(drone.isConnectionReady ? "Connected to Drone" : "Disconnected")
                    .font(.headline)
                    .foregroundColor(drone.isConnectionReady ? .green : .red)
            }
            .padding(.top, 20)
            // -------------------------------
            
            Text("Thrust: " + String(format: "%.2f", drone.droneData.thrust))
            
            Slider(value: Binding(
                get: { Double(drone.droneData.thrust) },
                set: { drone.droneData.thrust = Float($0) }
            ), in: 0...1)
            .padding()
            
            HStack {
                Button(action: {
                    drone.droneData.enabled = 1
                }) {
                    Text("Enabled")
                        .foregroundColor(enabledBool ? .white : .green)
                        .padding()
                        .frame(maxWidth: .infinity)
                        .background(
                            Circle()
                                .fill(enabledBool ? Color.green : Color.clear)
                        )
                }

                // Disable Button (right)
                Button(action: {
                    drone.droneData.enabled = 0
                }) {
                    Text("Disabled")
                        .foregroundColor(!enabledBool ? .white : .red)
                        .padding()
                        .frame(maxWidth: .infinity)
                        .background(
                            Circle()
                                .fill(!enabledBool ? Color.red : Color.clear)
                        )
                }
            }
            .frame(height: 150) // gives room for the circles
            .padding()
            Toggle("Angle Mode", isOn: Binding(
                get: { drone.droneData.angle != 0 },
                set: {drone.droneData.angle = $0 ? 1 : 0 }
            ))
            Toggle("Hold alt Mode", isOn: Binding(
                get: { drone.droneData.holdAlt != 0 },
                set: {drone.droneData.holdAlt = $0 ? 1 : 0 }
            ))
            Spacer()
        }
        .padding()
    }
}
