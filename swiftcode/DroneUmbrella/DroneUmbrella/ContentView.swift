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
            Text(String(drone.speed))
            Toggle(isOn: $drone.isArmed) {
                Text(drone.isArmed ? "ARMED (DANGER)" : "DISARMED")
                    .font(.title)
                    .foregroundColor(drone.isArmed ? .red : .green)
            }
            .padding()
        }
        .padding()
    }
}
