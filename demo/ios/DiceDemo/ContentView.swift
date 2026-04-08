import SwiftUI

private let dieTypes = [4, 6, 8, 10, 12, 16, 20, 32]

struct ContentView: View {
    @State private var selectedSides = 20
    @State private var hue: Double = 0.6        // starts blue-ish
    @State private var rollTrigger = 0

    var body: some View {
        ZStack {
            // Subtle dark background so the transparent die pops
            Color.black.ignoresSafeArea()

            VStack(spacing: 0) {
                // Die view — tapping rolls it
                DiceGameView(sides: selectedSides, hue: hue, rollTrigger: rollTrigger)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .onTapGesture { rollTrigger += 1 }

                controlPanel
            }
        }
    }

    private var controlPanel: some View {
        VStack(spacing: 16) {
            // Die type picker
            Picker("Die", selection: $selectedSides) {
                ForEach(dieTypes, id: \.self) { n in
                    Text("d\(n)").tag(n)
                }
            }
            .pickerStyle(.segmented)

            // Rainbow colour slider
            VStack(spacing: 6) {
                HStack {
                    Text("Color").foregroundStyle(.secondary)
                    Spacer()
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color(hue: hue, saturation: 0.85, brightness: 0.95))
                        .frame(width: 32, height: 20)
                }
                LinearGradientSlider(value: $hue)
            }

            Text("Tap the die to roll")
                .font(.caption)
                .foregroundStyle(.tertiary)
        }
        .padding(20)
        .background(.ultraThinMaterial)
    }
}

// MARK: - Rainbow slider

struct LinearGradientSlider: View {
    @Binding var value: Double

    private let gradient = LinearGradient(
        colors: (0..<20).map { Color(hue: Double($0) / 20, saturation: 0.85, brightness: 0.95) },
        startPoint: .leading, endPoint: .trailing
    )

    var body: some View {
        GeometryReader { geo in
            ZStack(alignment: .leading) {
                RoundedRectangle(cornerRadius: 6).fill(gradient).frame(height: 24)
                Circle()
                    .fill(.white)
                    .shadow(radius: 3)
                    .frame(width: 28, height: 28)
                    .offset(x: CGFloat(value) * (geo.size.width - 28))
                    .gesture(
                        DragGesture(minimumDistance: 0)
                            .onChanged { drag in
                                let x = drag.location.x
                                value = min(max(Double(x / geo.size.width), 0), 1)
                            }
                    )
            }
        }
        .frame(height: 28)
    }
}
