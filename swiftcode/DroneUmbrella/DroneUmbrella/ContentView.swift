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
        Text("Thrust: " + String(drone.droneData.thrust))
        VStack(spacing: 50) {
            Slider(value: Binding(
                get: { Double(drone.droneData.thrust) },
                set: { drone.droneData.thrust = Float($0) }
            ), in: 0...1, onEditingChanged: { editing in
                print("editing changed")
                drone.droneData.thrust = Float(0.5)
            })
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
            .padding()
        }
        .padding()
    }
}
