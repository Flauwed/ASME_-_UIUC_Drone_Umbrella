//
//  ContentView.swift
//  DroneUmbrella
//
//  Created by Jet Trommer on 4/9/26.
//

import SwiftUI

struct ContentView: View {
    @StateObject var drone = DroneController()
    
    var body: some View {
        VStack(spacing: 50) {
            Toggle(isOn: $drone.isArmed) {
                Text(drone.isArmed ? "ARMED (DANGER)" : "DISARMED")
                    .font(.title)
                    .foregroundColor(drone.isArmed ? .red : .green)
            }
            .padding()
            
            VStack {
                Text("Throttle: \(Int(drone.throttle * 100))%")
                Slider(value: $drone.throttle, in: 0.0...1.0)
                    .padding()
            }
        }
        .padding()
    }
}
