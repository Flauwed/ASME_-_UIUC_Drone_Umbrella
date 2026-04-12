//
//  LandingPage.swift
//  DroneUmbrella
//
//  Created by Michael Bauer on 4/12/26.
//

import SwiftUI

struct LandingPageView: View {
    @State var username = ""

    var body: some View {
        NavigationStack {
            Text("Username")
            TextField("", text: $username, prompt: Text("Username discoverable to local devices"))
                .textFieldStyle(.roundedBorder)
            NavigationLink(destination: GPSView(drone: DroneController(username: username))) {
                Image(systemName: "arrow.right.circle.fill")
                    .foregroundColor(Color(.gray))
            }
        }
    }
}
